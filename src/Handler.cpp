#include "Handler.hpp"

#include "Hook.hpp"
#include "Pool.hpp"
#include "Wrapper.hpp"
#include "target/PlatformTarget.hpp"

#include <algorithm>
#include <stack>
#include <iostream>

using namespace tulip::hook;

Handler::Handler(void* address, HandlerMetadata const& metadata) :
	m_address(address),
	m_metadata(metadata) {}

geode::Result<std::unique_ptr<Handler>> Handler::create(void* address, HandlerMetadata const& metadata) {
	auto ret = std::make_unique<Handler>(address, metadata);

	ret->m_content = new (std::nothrow) HandlerContent();
	if (!ret->m_content) {
		return geode::Err("Failed to allocate HandlerContent");
	}

	GEODE_UNWRAP_INTO(ret->m_handler, Target::get().allocateWritableArea(0x300));
	GEODE_UNWRAP_INTO(ret->m_trampoline, Target::get().allocateWritableArea(0x100));
	
	return geode::Ok(std::move(ret));
}

Handler::~Handler() {}

geode::Result<> Handler::init() {
	auto generator =
		Target::get().getHandlerGenerator(m_address, m_trampoline, m_handler, m_content, m_metadata);

	GEODE_UNWRAP_INTO(auto handler, generator->generateHandler());
	m_handlerSize = handler.m_size;

	GEODE_UNWRAP_INTO(auto minIntervener, generator->generateIntervener(0));
	GEODE_UNWRAP_INTO(auto trampoline, generator->generateTrampoline(minIntervener.size()));
	m_trampolineSize = trampoline.m_trampoline.m_size;

	GEODE_UNWRAP_INTO(m_modifiedBytes, generator->generateIntervener(trampoline.m_originalOffset));

	auto target = m_modifiedBytes.size();

	auto address = reinterpret_cast<uint8_t*>(Target::get().getRealPtr(m_address));
	m_originalBytes.insert(m_originalBytes.begin(), address, address + target);

	this->addOriginal();

	if (!Target::get().makeMemoryExecutable(m_handler, 0x300)) {
		return geode::Err("Failed to mark handler executable");
	}
	if (!Target::get().makeMemoryExecutable(m_trampoline, 0x100)) {
		return geode::Err("Failed to mark trampoline executable");
	}

	return geode::Ok();
}

void Handler::addOriginal() {
	auto metadata = HookMetadata{
		.m_priority = INT32_MAX,
	};
	static_cast<void>(this->createHook(Target::get().getRealPtrAs(m_trampoline, m_address), metadata));
}

HookHandle Handler::createHook(void* address, HookMetadata m_metadata) {
	static size_t s_nextHookHandle = 0;
	auto hook = HookHandle(++s_nextHookHandle);

	m_hooks.emplace(hook, std::make_unique<Hook>(address, m_metadata));
	m_handles.insert({address, hook});

	m_content->m_functions.push_back(address);

	this->reorderFunctions();

	return hook;
}

void Handler::removeHook(HookHandle const& hook) {
	if (m_hooks.count(hook) == 0) {
		return;
	}
	auto address = m_hooks[hook]->m_address;

	m_hooks.erase(hook);
	m_handles.erase(address);

	auto& vec = m_content->m_functions;

	vec.erase(std::remove(vec.begin(), vec.end(), address), vec.end());
}

void Handler::clearHooks() {
	m_hooks.clear();
	m_handles.clear();
	m_content->m_functions.clear();

	this->addOriginal();
}

void Handler::updateHookMetadata(HookHandle const& hook, HookMetadata const& metadata) {
	if (m_hooks.count(hook) == 0) {
		return;
	}
	m_hooks.at(hook)->m_metadata = metadata;
	this->reorderFunctions();
}

void Handler::reorderFunctions() {
	auto& vec = m_content->m_functions;
	std::sort(vec.begin(), vec.end(), [this](auto const a, auto const b) {
		return (m_hooks.at(m_handles[a])->m_metadata.m_priority < m_hooks.at(m_handles[b])->m_metadata.m_priority);
	});
}

geode::Result<> Handler::interveneFunction() {
	return Target::get().writeMemory(
		Target::get().getRealPtr(m_address),
		static_cast<void*>(m_modifiedBytes.data()),
		m_modifiedBytes.size()
	);
}

geode::Result<> Handler::restoreFunction() {
	return Target::get().writeMemory(
		Target::get().getRealPtr(m_address),
		static_cast<void*>(m_originalBytes.data()),
		m_originalBytes.size()
	);
}

static thread_local std::stack<size_t> s_indexStack;
static thread_local std::stack<HandlerContent*> s_addressStack;
static thread_local std::stack<void*> s_dataStack;

void Handler::incrementIndex(HandlerContent* content) {
	if (s_addressStack.size() == 0 || s_addressStack.top() != content) {
		s_addressStack.push(content);
		s_indexStack.push(0);
	}
	else {
		++s_indexStack.top();
	}
}

void Handler::decrementIndex() {
	if (s_indexStack.top() == 0) {
		s_addressStack.pop();
		s_indexStack.pop();
	}
	else {
		--s_indexStack.top();
	}
}

void* Handler::getNextFunction(HandlerContent* content) {
	auto& vec = content->m_functions;
	auto ret = vec[s_indexStack.top() % vec.size()];
	return ret;
}

void* Handler::popData() {
	auto ret = s_dataStack.top();
	s_dataStack.pop();
	return ret;
}

void Handler::pushData(void* data) {
	s_dataStack.push(data);
}

static thread_local std::stack<size_t> s_wrapperIndexStack;
static thread_local std::stack<WrapperContent*> s_wrapperContentStack;

void Handler::incrementWrapperIndex(WrapperContent* content) {
	if (s_wrapperContentStack.empty() || s_wrapperContentStack.top() != content) {
		s_wrapperContentStack.push(content);
		s_wrapperIndexStack.push(0);
	} else {
		++s_wrapperIndexStack.top();
	}
}

void Handler::decrementWrapperIndex() {
	if (s_wrapperIndexStack.top() == 0) {
		s_wrapperContentStack.pop();
		s_wrapperIndexStack.pop();
	} else {
		--s_wrapperIndexStack.top();
	}
}

void* Handler::getNextWrapperFunction(WrapperContent* content) {
	auto& vec = content->m_functions;
	auto ret = vec[s_wrapperIndexStack.top() % vec.size()];
	return ret;
}