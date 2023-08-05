#include "X64Assembler.hpp"

using namespace tulip::hook;

uint8_t regv(X64Register reg) {
	return static_cast<uint8_t>(reg);
}

uint8_t regv(X64Pointer ptr) {
	return regv(ptr.reg);
}

uint8_t lowerv(X64Register reg, uint8_t offset) {
	return (regv(reg) < 0x8) << offset;
}

uint8_t lowerv(X64Pointer ptr, uint8_t offset) {
	return (regv(ptr) < 0x8) << offset;
}

void rex(X64Assembler* ass, X64Register reg, X64Register reg2, bool wide) {
	auto rexv = 0x40 | lowerv(reg, 0) | lowerv(reg2, 2) | (wide << 3);
	if (rexv != 0x40) {
		ass->write8(rexv);
	}
}

void rex(X64Assembler* ass, X64Pointer ptr, X64Register reg, bool wide) {
	auto rexv = 0x40 | lowerv(ptr, 0) | lowerv(reg, 2) | (wide << 3);
	if (rexv != 0x40) {
		ass->write8(rexv);
	}
}

X86Register x86reg(X64Register reg) {
	return static_cast<X86Register>(regv(reg));
}

X86Pointer x86ptr(X64Pointer ptr) {
	return {x86reg(ptr.reg), ptr.offset};
}

X64Assembler::X64Assembler(uint64_t baseAddress) :
	X86Assembler(baseAddress) {}

X64Assembler::~X64Assembler() {}

void X64Assembler::updateLabels() {
	for (auto const& update : m_labelUpdates) {
		this->rewrite32(update.m_address, m_labels[update.m_name] - update.m_address - 4);
	}
}

using enum X64Register;

void X64Assembler::nop() {
	X86Assembler::nop();
}

void X64Assembler::add(X64Register reg, uint32_t value) {
	rex(this, reg, RAX, true);
	X86Assembler::add(x86reg(reg), value);
}

void X64Assembler::sub(X64Register reg, uint32_t value) {
	rex(this, reg, RAX, true);
	X86Assembler::sub(x86reg(reg), value);
}

void X64Assembler::jmp(X64Register reg) {
	rex(this, reg, RAX, false);
	X86Assembler::jmp(x86reg(reg));
}

void X64Assembler::jmp(uint64_t address) {
	X86Assembler::jmp(address);
}

void X64Assembler::call(X64Register reg) {
	rex(this, reg, RAX, false);
	X86Assembler::call(x86reg(reg));
}

void X64Assembler::lea(X64Register reg, std::string const& label) {
	rex(this, RAX, reg, true);
	X86Assembler::lea(x86reg(reg), label);
}

void X64Assembler::movaps(X64Register reg, X64Pointer ptr) {
	rex(this, ptr, RAX, false);
	X86Assembler::movaps(x86reg(reg), x86ptr(ptr));
}

void X64Assembler::movaps(X64Pointer ptr, X64Register reg) {
	rex(this, ptr, RAX, false);
	X86Assembler::movaps(x86ptr(ptr), x86reg(reg));
}

void X64Assembler::mov(X64Register reg, uint32_t value) {
	rex(this, reg, RAX, true);
	X86Assembler::mov(x86reg(reg), value);
}

void X64Assembler::mov(X64Register reg, X64Pointer ptr) {
	rex(this, ptr, reg, true);
	X86Assembler::mov(x86reg(reg), x86ptr(ptr));
}

void X64Assembler::mov(X64Pointer ptr, X64Register reg) {
	rex(this, ptr, reg, true);
	X86Assembler::mov(x86ptr(ptr), x86reg(reg));
}

void X64Assembler::mov(X64Register reg, X64Register reg2) {
	rex(this, reg, reg2, true);
	X86Assembler::mov(x86reg(reg), x86reg(reg2));
}

void X64Assembler::mov(X64Register reg, std::string const& label) {
	rex(this, RAX, reg, true);
	X86Assembler::mov(x86reg(reg), label);
}