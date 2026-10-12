// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
// MAME's 68000 (cpu/m68000/m68000.cpp), reduced to the core (see m68000.h): execution, input
// lines, reset, the decode table. The code kept is MAME's.

#include "emu.h"
#include "m68000.h"

m68000_device::m68000_device(m68000_bus &bus, u32 clock)
	: cpu_device(machine_config{}, nullptr, "m68000", nullptr, clock)
{
	m_program.bus = m_opcodes.bus = m_cpu_space.bus = &bus;
	m_program.kind = m68000_bus::PROGRAM;
	m_opcodes.kind = m68000_bus::OPCODES;
	m_cpu_space.kind = m68000_bus::CPU_SPACE;
}

void m68000_device::abort_access(u32 reason)
{
	m_post_run = reason;
	m_post_run_cycles = m_icount;
	m_icount = 0;
}

void m68000_device::do_post_run()
{
	m_icount = m_post_run_cycles;
	m_post_run_cycles = 0;
	switch(m_post_run) {
	case PR_BERR:
		m_inst_state = S_BUS_ERROR;
		m_inst_substate = 0;
		m_icount -= 10;
		break;
	}
	m_post_run = 0;
}

void m68000_device::execute_run()
{
	m_icount -= m_count_before_instruction_step;
	if(m_icount < 0) {
		m_count_before_instruction_step = -m_icount;
		m_icount = 0;
	} else
		m_count_before_instruction_step = 0;

	for(;;) {
		if(m_icount > 0 && m_inst_substate)
			(this->*(m_handlers_p[m_inst_state]))();

		while(m_icount > 0) {
			if(m_inst_state >= S_first_instruction) {
				m_ipc = m_pc - 2;
				m_irdi = m_ird;

				debugger_instruction_hook(m_ipc);
			}
			(this->*(m_handlers_f[m_inst_state]))();
		}

		if(m_post_run)
			do_post_run();
		else
			break;
	}

	if(m_icount < 0) {
		m_count_before_instruction_step = -m_icount;
		m_icount = 0;
	}
}

void m68000_device::device_start()
{
	init_decode_table();

	m_handlers_f = s_handlers_df;
	m_handlers_p = s_handlers_dp;

	memset(m_da, 0, sizeof(m_da));
	m_ipc = 0;
	m_pc = 0;
	m_au = 0;
	m_at = 0;
	m_aob = 0;
	m_dt = 0;
	m_int_vector = 0;
	m_sp = 0;
	m_bcount = 0;
	m_count_before_instruction_step = 0;
	m_t = 0;
	m_movems = 0;
	m_isr = 0;
	m_sr = 0;
	m_new_sr = 0;
	m_dbin = 0;
	m_dbout = 0;
	m_edb = 0;
	m_irc = 0;
	m_ir = 0;
	m_ird = 0;
	m_ftu = 0;
	m_aluo = 0;
	m_alue = 0;
	m_alub = 0;
	m_movemr = 0;
	m_irdi = 0;
	m_base_ssw = 0;
	m_ssw = 0;
	m_dcr = 0;

	m_virq_state = 0;
	m_nmi_pending = 0;
	m_int_level = 0;
	m_int_next_state = 0;
	m_nmi_uses_generic = false;
	m_disable_interrupt_callback = false;
	m_last_vpa_time = 0;

	m_inst_state = 0;
	m_inst_substate = 0;
	m_next_state = 0;
	m_post_run = 0;
	m_post_run_cycles = 0;

	state_add(STATE_GENPCBASE, "CURPC", m_ipc);
	state_add(STATE_GENPC,     "PC",    m_pc);
	state_add(1,               "SR",    m_sr);

	set_icountptr(m_icount);
}

void m68000_device::device_reset()
{
	m_inst_state = S_RESET;
	m_inst_substate = 0;
	m_count_before_instruction_step = 0;
	m_post_run = 0;
	m_post_run_cycles = 0;
	m_last_vpa_time = 0;
	update_user_super();
}

void m68000_device::update_user_super()
{
	// the same spaces in user and supervisor mode on this board
	if(m_sr & SR_S)
		m_sp = 16;
	else
		m_sp = 15;
}

void m68000_device::execute_set_input(int inputnum, int state)
{
	if(inputnum == INPUT_LINE_NMI) {
		inputnum = 7;
		m_nmi_uses_generic = true;
	} else if(inputnum == 7)
		m_nmi_uses_generic = false;

	if(inputnum > (m_interrupt_mixer ? 7 : 3))
		return;

	u32 old_level = m_int_level;
	u32 vstate = m_virq_state;
	u32 blevel;

	if(state == ASSERT_LINE)
		vstate |= 1 << inputnum;
	else
		vstate &= ~(1 << inputnum);
	m_virq_state = vstate;

	if(m_interrupt_mixer) {
		for(blevel = 7; blevel > 0; blevel--)
			if(vstate & (1 << blevel))
				break;
	} else
		blevel = vstate;

	m_int_level = blevel;

	/* A transition from < 7 to 7 always interrupts (NMI) */
	/* Note: Level 7 can also level trigger like a normal IRQ */
	// FIXME: This may cause unintended level 7 interrupts if one or two IPL lines are asserted
	// immediately before others are cleared. The actual 68000 imposes an input hold time.
	if(old_level != 7 && m_int_level == 7)
		m_nmi_pending = true;

	update_interrupt();
}

void m68000_device::update_interrupt()
{
	if(m_nmi_pending)
		m_int_next_state = (7 << 24) | S_INTERRUPT;
	else if(m_int_level > ((m_sr >> 8) & 7))
		m_int_next_state = (m_int_level << 24) | S_INTERRUPT;
	else
		m_int_next_state = 0;
}

void m68000_device::init_decode_table()
{
	m_decode_table.resize(0x10000, S_ILLEGAL);

	for(int i=0; s_packed_decode_table[i].mask; i++) {
		u16 value = s_packed_decode_table[i].value;
		u16 mask = s_packed_decode_table[i].mask;
		u16 state = s_packed_decode_table[i].state;

		u16 cvalue = 0;
		do {
			if(m_decode_table[value | cvalue] == S_ILLEGAL)
				m_decode_table[value | cvalue] = state;
			cvalue = ((cvalue | mask) + 1) & ~mask;
		} while(cvalue);
	}
}

void m68000_device::set_ftu_const()
{
	switch(m_ird >> 12) {
	case 0x4: m_ftu = 0x80; break;
	case 0x5: case 0xe: m_ftu = (m_ird >> 9) & 7; if(!m_ftu) m_ftu = 8; break;
	case 0x6: case 0x7: m_ftu = s8(m_ird); break;
	case 0x8: case 0xc: m_ftu = 0xf; break;
	default: m_ftu = 0; break;
	}
}

void m68000_device::start_interrupt_vector_lookup()
{
	// flag for berr -> spurious

	int level = m_next_state >> 24;
	if(!m_disable_interrupt_callback) {
		if(m_interrupt_mixer)
			standard_irq_callback(level == 7 && m_nmi_uses_generic ? INPUT_LINE_NMI : level, m_pc);
		else {
			for(int i=0; i<3; i++)
				if(level & (1<<i))
					standard_irq_callback(i, m_pc);
		}
	}

	// Clear the nmi flag
	if(level == 7) {
		m_nmi_pending = false;
		update_interrupt();
	}
}

void m68000_device::end_interrupt_vector_lookup()
{
	m_int_vector = (m_edb & 0xff) << 2;
	m_int_next_state = 0;
}
