#ifdef SYSTEM_CPP

#define DEBUG_FILE_OUT_NAME "bsnes_debug.out"

Debugger debugger;

bool Debugger::Breakpoint::operator==(const uint8& data) const {
  if (this->data < 0) return true;
  switch (compare) {
  case Compare::Equal:        return data == this->data;
  case Compare::NotEqual:     return data != this->data;
  case Compare::Less:         return data <  this->data;
  case Compare::LessEqual:    return data <= this->data;
  case Compare::Greater:      return data >  this->data;
  case Compare::GreaterEqual: return data >= this->data;
  }
  return false;
}

bool Debugger::Breakpoint::operator!=(const uint8& data) const {
  return !operator==(data);
}

int Debugger::breakpoint_exec_command(Breakpoint::Source source, const string &command, const lstring &params, bool &mute, bool &cancel) {
  if (command[0] == '$') { return hex(substr(command, 1, command.length() - 1)); }
  if (command[0] >= '0' && command[0] <= '9') { int result=0; strint(command, result); return result; }

  if (source == Breakpoint::Source::CPUBus) {
    if (command == "A") { return cpu.regs.a & (cpu.regs.p.m ? 0xFF : 0xFFFF); }
    if (command == "X") { return cpu.regs.x & (cpu.regs.p.x ? 0xFF : 0xFFFF); }
    if (command == "Y") { return cpu.regs.y & (cpu.regs.p.x ? 0xFF : 0xFFFF); }
    if (command == "A16") { return cpu.regs.a; }
    if (command == "X16") { return cpu.regs.x; }
    if (command == "Y16") { return cpu.regs.y; }
    if (command == "DB") { return cpu.regs.db; }
    if (command == "D") { return cpu.regs.d; }
    if (command == "S") { return cpu.regs.s; }
    if (command == "PC") { return cpu.regs.pc; }
    if (command == "P") { return cpu.regs.p; }
  } else if (source == Breakpoint::Source::SA1Bus) {
    if (command == "A") { return sa1.regs.a & (sa1.regs.p.m ? 0xFF : 0xFFFF); }
    if (command == "X") { return sa1.regs.x & (sa1.regs.p.x ? 0xFF : 0xFFFF); }
    if (command == "Y") { return sa1.regs.y & (sa1.regs.p.x ? 0xFF : 0xFFFF); }
    if (command == "A16") { return sa1.regs.a; }
    if (command == "X16") { return sa1.regs.x; }
    if (command == "Y16") { return sa1.regs.y; }
    if (command == "DB") { return sa1.regs.db; }
    if (command == "D") { return sa1.regs.d; }
    if (command == "S") { return sa1.regs.s; }
    if (command == "PC") { return sa1.regs.pc; }
    if (command == "P") { return sa1.regs.p; }
  }

  if (command == "vcounter") { return ppu.vcounter(); }
  if (command == "frame") { return cpu.framecounter(); }
  if (command == "clock") { return cpu.clocks(); }

  uint32_t left = params.size() >= 1 ? breakpoint_command(source, params[0], mute, cancel) : 0;
  if (command == "muteif") {
    cancel = !!left;
    mute = true;
    return 0;
  }
  if (command == "byte") {
    SNES::debugger.bus_access = true;
    uint8_t data = SNES::debugger.read(SNES::Debugger::MemorySource::CPUBus, left);
    SNES::debugger.bus_access = false;
    return data;
  }
  if (command == "word") {
    SNES::debugger.bus_access = true;
    uint8_t bl = SNES::debugger.read(SNES::Debugger::MemorySource::CPUBus, left);
    uint8_t bh = SNES::debugger.read(SNES::Debugger::MemorySource::CPUBus, left + 1);
    SNES::debugger.bus_access = false;
    return bl | bh << 8;
  }
  if (command == "put_byte") {
    debug_out_file = debug_out_file ? debug_out_file : fopen(DEBUG_FILE_OUT_NAME, "w");
    if (debug_out_file) fwrite(&left, 1, 1, debug_out_file);
    return left;
  }
  if (command == "put_word") {
    debug_out_file = debug_out_file ? debug_out_file : fopen(DEBUG_FILE_OUT_NAME, "w");
    if (debug_out_file) fwrite(&left, 2, 1, debug_out_file);
    return left;
  }
  if (command == "put_int") {
    debug_out_file = debug_out_file ? debug_out_file : fopen(DEBUG_FILE_OUT_NAME, "w");
    if (debug_out_file) fwrite(&left, 4, 1, debug_out_file);
    return left;
  }


  uint32_t right = params.size() >= 2 ? breakpoint_command(source, params[1], mute, cancel) : 0;
  if (command == "add") { return left + right; }
  if (command == "sub") { return left - right; }
  if (command == "and") { return left & right; }
  if (command == "or") { return left | right; }
  if (command == "equ") { return (left == right) ? 1 : 0; }
  if (command == "long") { return left | right << 16; }

  puts(string("Unknown debug command: ").append(command));
  return 0;
}

int Debugger::breakpoint_command(Breakpoint::Source source, const string &line, bool &mute, bool &cancel) {
  optional<unsigned> optParamStart(line.position("("));
  lstring params;
  string command(line);

  if ((bool)optParamStart) {
    unsigned paramStart = optParamStart();
    command = substr(line, 0, paramStart).trim();

    const char *lineText = line;
    int lineLength = line.length();
    int depth = 1;
    int left = paramStart + 1;
    for (int right = paramStart + 1; right < lineLength; right++) {
      char c = lineText[right];

      if (c == '(') {
        depth++;
      } else if (c == ')') {
        if (--depth == 0) {
          if (left != right) { params.append(substr(lineText, left, right - left).trim()); }
          left = lineLength + 1;
          break;
        }
      } else if (c == ',' && depth == 1) {
        if (left != right) { params.append(substr(lineText, left, right - left).trim()); }
        left = right + 1;
      }
    }

    if (left + 1 < lineLength) { params.append(substr(lineText, left, lineLength - left).trim()); }
  }

  return breakpoint_exec_command(source, command, params, mute, cancel);
}

void Debugger::breakpoint_notify(Breakpoint::Source source, unsigned addr, const string &name) {
  if (!name) {
    puts(string("Breakpoint hit at ").append(hex<6>(addr)));
    return;
  }

  string result;
  const char *data = name;
  int len = name.length();
  int left = 0;
  for (int right=0; right<len; right++) {
    if (data[right] == '{') {
      if (left != right) { result << substr(data, left, right - left); }
      left = right;
    } else if (data[right] == '}') {
      if (data[left] == '{') {
        bool mute = false;
        bool cancel = false;
        int number = breakpoint_command(source, substr(data, left + 1, right - left - 1), mute, cancel);
        if (cancel) {
          return;
        }
        if (!mute) {
          result << hex(number);
        }
        left = right + 1;
      }
    }
  }

  if (left + 1 < len) { result << substr(data, left, len - left); }

  puts(result);
}

void Debugger::breakpoint_test(Debugger::Breakpoint::Source source, Debugger::Breakpoint::Mode mode, unsigned addr, uint8 data) {
  for(unsigned i = 0; i < breakpoint.size(); i++) {

    if((breakpoint[i].mode & (unsigned)mode) == 0) continue;
    if(breakpoint[i].source != source) continue;
    if(breakpoint[i] != data) continue;
    
    // account for address mirroring on the S-CPU and SA-1 (and other) buses
    // (with 64kb granularity for ranged breakpoints)
    unsigned addr_start = (breakpoint[i].addr & 0xff0000) | (addr & 0xffff);
    if (addr_start < breakpoint[i].addr) {
      addr_start += 1<<16;
    }
    unsigned addr_end = breakpoint[i].addr;
    if (breakpoint[i].addr_end > breakpoint[i].addr) {
      addr_end = breakpoint[i].addr_end;
    }
    
    for (; addr_start <= addr_end; addr_start += 1<<16) {
      if (source == Debugger::Breakpoint::Source::CPUBus) {
        if (bus.is_mirror(addr_start, addr)) break;
      } else if (source == Debugger::Breakpoint::Source::SA1Bus) {
        if (sa1bus.is_mirror(addr_start, addr)) break;
      } else if (source == Debugger::Breakpoint::Source::SFXBus) {
        if (superfxbus.is_mirror(addr_start, addr)) break;
      } else {
        if (addr_start == addr) break;
      }
    }
    if (addr_start > addr_end) continue;

    if (breakpoint[i].notify_only) {
      breakpoint_notify(source, addr, breakpoint[i].name);
      continue;
    }
    
    breakpoint[i].counter++;
    breakpoint_hit = i;
    break_event = BreakEvent::BreakpointHit;
    scheduler.exit(Scheduler::ExitReason::DebuggerEvent);
    break;
  }
}

uint8 Debugger::read(Debugger::MemorySource source, unsigned addr) {
  switch(source) {
    case MemorySource::CPUBus: {
      return bus.read(addr & 0xffffff);
    } break;

    case MemorySource::APUBus: {
      return smp.op_debugread(addr & 0xffff);
    } break;

    case MemorySource::APURAM: {
      return memory::apuram.read(addr & 0xffff);
    } break;
    
    case MemorySource::DSP: {
      return dsp.read(addr & 0x7f);
    } break;

    case MemorySource::VRAM: {
      return memory::vram.read(addr & 0x3ffff);
    } break;

    case MemorySource::OAM: {
      if(addr & 0x0200) return memory::oam.read(0x0200 + (addr & 0x1f));
      return memory::oam.read(addr & 0x01ff);
    } break;

    case MemorySource::CGRAM: {
      return memory::cgram.read(addr & 0x01ff);
    } break;
    
    case MemorySource::CartROM: {
      if (addr < memory::cartrom.size())
        return memory::cartrom.read(addr & 0xffffff);
    } break;
    
    case MemorySource::CartRAM: {
      if (addr < memory::cartram.size())
        return memory::cartram.read(addr & 0xffffff);
    } break;
    
    case MemorySource::SA1Bus: {
      if (cartridge.has_sa1())
        return sa1bus.read(addr & 0xffffff);
    } break;
    
    case MemorySource::SFXBus: {
      if (cartridge.has_superfx())
        return superfxbus.read(addr & 0xffffff);
    } break;
    
    case MemorySource::SGBBus: {
      if (cartridge.mode() == Cartridge::Mode::SuperGameBoy)
        return supergameboy.read_gb(addr & 0xffff);
    } break;
    
    case MemorySource::SGBROM: {
      if (addr < memory::gbrom.size())
        return memory::gbrom.read(addr & 0xffffff);
    } break;
    
    case MemorySource::SGBRAM: {
      if (addr < memory::gbram.size())
        return memory::gbram.read(addr & 0xffffff);
    } break;
  }

  return 0x00;
}

void Debugger::write(Debugger::MemorySource source, unsigned addr, uint8 data) {
  switch(source) {
    case MemorySource::CPUBus: {
      bus.write(addr & 0xffffff, data);
    } break;
    
    case MemorySource::APUBus:
    case MemorySource::APURAM: {
      memory::apuram.write(addr & 0xffff, data);
    } break;

    case MemorySource::DSP: {
      dsp.write(addr & 0x7f, data);
    } break;

    case MemorySource::VRAM: {
      memory::vram.write(addr & 0x3ffff, data);
    } break;

    case MemorySource::OAM: {
      if(addr & 0x0200) memory::oam.write(0x0200 + (addr & 0x1f), data);
      else memory::oam.write(addr & 0x01ff, data);
    } break;

    case MemorySource::CGRAM: {
      memory::cgram.write(addr & 0x01ff, data);
    } break;
    
    case MemorySource::CartROM: {
      if (addr < memory::cartrom.size()) {
        memory::cartrom.write(addr & 0xffffff, data);
      }
    } break;
    
    case MemorySource::CartRAM: {
      if (addr < memory::cartram.size())
        memory::cartram.write(addr & 0xffffff, data);
    } break;
    
    case MemorySource::SA1Bus: {
      if (cartridge.has_sa1()) sa1bus.write(addr & 0xffffff, data);
    } break;
    
    case MemorySource::SFXBus: {
      if (cartridge.has_superfx()) superfxbus.write(addr & 0xffffff, data);
    } break;
    
    case MemorySource::SGBBus: {
      if (cartridge.mode() == Cartridge::Mode::SuperGameBoy)
        supergameboy.write_gb(addr & 0xffff, data);
    } break;
    
    case MemorySource::SGBROM: {
      if (addr < memory::gbrom.size())
        memory::gbrom.write(addr & 0xffffff, data);
    } break;
    
    case MemorySource::SGBRAM: {
      if (addr < memory::gbram.size())
        memory::gbram.write(addr & 0xffffff, data);
    } break;
  }
}

Debugger::Debugger() {
  break_event = BreakEvent::None;

  breakpoint_hit = 0;

  step_cpu = false;
  step_smp = false;
  step_sa1 = false;
  step_sfx = false;
  bus_access = false;
  break_on_wdm = false;
  break_on_brk = false;

  step_type = StepType::None;
}

Debugger::~Debugger() {
  if (debug_out_file) {
    fclose(debug_out_file);
  }
}

#endif
