/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2000 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *
 */

#include "st20.hpp"
#include <diskio.hpp>

//--------------------------------------------------------------------------
static const char *const register_names[] =
{
  "Areg",       // Evaluation stack register A
  "Breg",       // Evaluation stack register B
  "Creg",       // Evaluation stack register C
  "Iptr",       // Instruction pointer register, pointing to the next instruction to be executed
  "Status",     // Status register
  "Wptr",       // Work space pointer, pointing to the stack of the currently executing process
  "Tdesc",      // Task descriptor
  "IOreg",      // Input and output register
  "cs", "ds",
};

//--------------------------------------------------------------------------
static const uchar ret0[] = { 0x23, 0x22 }; // eret
static const uchar ret1[] = { 0x24, 0xF5 }; // altend
static const uchar ret2[] = { 0x20, 0xF3 }; // endp
static const uchar ret3[] = { 0x61, 0xFF }; // iret
static const uchar ret4[] = { 0x68, 0xFD }; // reboot
static const uchar ret5[] = { 0x62, 0xFE }; // restart
static const uchar ret6[] = { 0x22, 0xF0 }; // ret
static const uchar ret7[] = { 0x60, 0xFB }; // tret

static const bytes_t retcodes1[] =
{
  { qnumber(ret0), ret0, },
  { 0, NULL }
};

static const bytes_t retcodes4[] =
{
  { qnumber(ret1), ret1, },
  { qnumber(ret2), ret2, },
  { qnumber(ret3), ret3, },
  { qnumber(ret4), ret4, },
  { qnumber(ret5), ret5, },
  { qnumber(ret6), ret6, },
  { qnumber(ret7), ret7, },
  { 0, NULL }
};

//-----------------------------------------------------------------------
//      Hypthetical assembler
//-----------------------------------------------------------------------
static const asm_t hypasm =
{
  ASH_HEXF0     // 1234h
 |ASD_DECF0     // 1234
 |ASB_BINF0     // 1010b
 |ASO_OCTF0     // 1234o
 |AS_COLON      // create colons after data names
 |AS_ONEDUP,    // one array definition per line
  0,
  "Hypthetical assembler",
  0,
  NULL,         // header lines
  "org",        // org
  "end",        // end

  ";",          // comment string
  '\"',         // string delimiter
  '\'',         // char delimiter
  "'\"",        // special symbols in char and string constants

  "db",         // ascii string directive
  "db",         // byte directive
  "dw",         // word directive
  "dd",         // double words
  "dq",         // qwords
  NULL,         // oword  (16 bytes)
  NULL,         // float  (4 bytes)
  NULL,         // double (8 bytes)
  NULL,         // tbyte  (10/12 bytes)
  NULL,         // packed decimal real
  NULL,         // arrays (#h,#d,#v,#s(...)
  "ds %s",      // uninited arrays
  "equ",        // equ
  NULL,         // 'seg' prefix (example: push seg seg001)
  "$",          // current IP (instruction pointer)
  NULL,         // func_header
  NULL,         // func_footer
  "public",     // "public" name keyword
  NULL,         // "weak"   name keyword
  "extrn",      // "extrn"  name keyword
                // .extern directive requires an explicit object size
  NULL,         // "comm" (communal variable)
  NULL,         // get_type_name
  NULL,         // "align" keyword
  '(', ')',     // lbrace, rbrace
  "mod",        // mod
  "&",          // and
  "|",          // or
  "^",          // xor
  "not",        // not
  "<<",         // shl
  ">>",         // shr
  NULL,         // sizeof
};

static const asm_t *const asms[] = { &hypasm, NULL };

//--------------------------------------------------------------------------
static const char cfgname[] = "st20.cfg";

//--------------------------------------------------------------------------
const char *st20_t::set_idp_options(
        const char *keyword,
        int /*value_type*/,
        const void * /*value*/,
        bool /*idb_loaded*/)
{
  if ( keyword != NULL )
    return IDPOPT_BADKEY;
  choose_ioport_device(&ioh.device, cfgname);
  return IDPOPT_OK;
}

//----------------------------------------------------------------------
// This old-style callback only returns the processor module object.
static ssize_t idaapi notify(void *, int msgid, va_list)
{
  if ( msgid == processor_t::ev_get_procmod )
    return size_t(new st20_t);
  return 0;
}

//--------------------------------------------------------------------------
ssize_t idaapi idb_listener_t::on_event(ssize_t code, va_list /*va*/)
{
  switch ( code )
  {
    case idb_event::savebase:
    case idb_event::closebase:
      pm.helper.supset(0, pm.ioh.device.c_str());
      break;
  }
  return 0;
}

//--------------------------------------------------------------------------
ssize_t idaapi st20_t::on_event(ssize_t msgid, va_list va)
{
  int code = 0;
  switch ( msgid )
  {
    case processor_t::ev_init:
      hook_event_listener(HT_IDB, &idb_listener, &LPH);
      helper.create("$ st20");
      break;

    case processor_t::ev_term:
      unhook_event_listener(HT_IDB, &idb_listener);
      break;

    case processor_t::ev_ending_undo:
      procnum = ph.get_proc_index();
      if ( isc4() )
        ph.retcodes = retcodes4;
      //fall through
    case processor_t::ev_oldfile:
      helper.supstr(&ioh.device, 0);
      break;

    case processor_t::ev_newprc:   // new processor type
      procnum = va_arg(va, int);
      // bool keep_cfg = va_argi(va, bool);
      if ( isc4() )
        ph.retcodes = retcodes4;
      break;

    case processor_t::ev_is_jump_func:
      {
        const func_t *pfn = va_arg(va, const func_t *);
        ea_t *jump_target = va_arg(va, ea_t *);
        return is_jump_func(pfn, jump_target);
      }

    case processor_t::ev_is_sane_insn:
      {
        const insn_t *insn = va_arg(va, insn_t *);
        int no_crefs = va_arg(va, int);
        return is_sane_insn(*insn, no_crefs) == 1 ? 1 : -1;
      }

    case processor_t::ev_may_be_func:
      {
        const insn_t *insn = va_arg(va, insn_t *);
        return may_be_func(*insn);
      }

    case processor_t::ev_out_header:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        st20_header(*ctx);
        return 1;
      }

    case processor_t::ev_out_footer:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        st20_footer(*ctx);
        return 1;
      }

    case processor_t::ev_out_segstart:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        segment_t *seg = va_arg(va, segment_t *);
        st20_segstart(*ctx, seg);
        return 1;
      }

    case processor_t::ev_out_segend:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        segment_t *seg = va_arg(va, segment_t *);
        st20_segend(*ctx, seg);
        return 1;
      }

    case processor_t::ev_ana_insn:
      {
        insn_t *out = va_arg(va, insn_t *);
        return st20_ana(out);
      }

    case processor_t::ev_emu_insn:
      {
        const insn_t *insn = va_arg(va, const insn_t *);
        return st20_emu(*insn) ? 1 : -1;
      }

    case processor_t::ev_out_insn:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        out_insn(*ctx);
        return 1;
      }

    case processor_t::ev_out_operand:
      {
        outctx_t *ctx = va_arg(va, outctx_t *);
        const op_t *op = va_arg(va, const op_t *);
        return out_opnd(*ctx, *op) ? 1 : -1;
      }

    case processor_t::ev_set_idp_options:
      {
        const char *keyword = va_arg(va, const char *);
        int value_type = va_arg(va, int);
        const char *value = va_arg(va, const char *);
        const char **errmsg = va_arg(va, const char **);
        bool idb_loaded = va_argi(va, bool);
        const char *ret = set_idp_options(keyword, value_type, value, idb_loaded);
        if ( ret == IDPOPT_OK )
          return 1;
        if ( errmsg != NULL )
          *errmsg = ret;
        return -1;
      }

    case processor_t::ev_is_align_insn:
      {
        ea_t ea = va_arg(va, ea_t);
        return is_align_insn(ea);
      }

    default:
      break;
  }
  return code;
}

//-----------------------------------------------------------------------
#define FAMILY "SGS-Thomson ST20:"
static const char *const shnames[] = { "st20", "st20c4", NULL };
static const char *const lnames[] =
{
  FAMILY"SGS-Thomson ST20/C1",
  "SGS-Thomson ST20/C2-C4",
  NULL
};

//-----------------------------------------------------------------------
//      Processor Definition
//-----------------------------------------------------------------------
processor_t LPH =
{
  IDP_INTERFACE_VERSION,  // version
  PLFM_ST20,              // id
                          // flag
    PRN_HEX
  | PR_RNAMESOK,
                          // flag2
  PR2_IDP_OPTS,         // the module has processor-specific configuration options
  8,                      // 8 bits in a byte for code segments
  8,                      // 8 bits in a byte for other segments

  shnames,
  lnames,

  asms,

  notify,

  register_names,       // Register names
  qnumber(register_names), // Number of registers

  cs,                   // first
  ds,                   // last
  2,                    // size of a segment register
  cs, ds,

  NULL,                 // No known code start sequences
  retcodes1,

  ST20_null,
  ST20_last,
  Instructions,         // instruc
  0,                    // int tbyte_size;  -- doesn't exist
  { 0, 7, 15, 0 },      // char real_width[4];
                        // number of symbols after decimal point
                        // 2byte float (0-does not exist)
                        // normal float
                        // normal double
                        // long double
  ST20_eret,            // Icode of return instruction. It is ok to give any of possible return instructions
};
