/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2020 Hex-Rays
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef _NAME_HPP
#define _NAME_HPP

#include <ida.hpp>

/*! \file name.hpp

  \brief Functions that deal with names.

  A non-tail address of the program may have a name.
  Tail addresses (i.e. the addresses in the middle of an instruction or
  data item) cannot have names.
*/

class func_t;                   // funcs.hpp
typedef uchar color_t;          // lines.hpp

/// Maximum length of a name in IDA (with the trailing zero)
#define MAXNAMELEN      512


/// Name prefix used by IDA for the imported functions
#define FUNC_IMPORT_PREFIX "__imp_"


/// Set or delete name of an item at the specified address.
/// An item can be anything: instruction, function, data byte, word, string,
/// structure, etc...
/// Include name into the list of names.
/// \param ea    linear address.
///              do nothing if ea is not valid (return 0).
///              tail bytes can't have names.
/// \param name  new name.
///                - NULL: do nothing (return 0).
///                - ""  : delete name.
///                - otherwise this is a new name.
/// \param flags \ref SN_.
///              If a bit is not specified, then the corresponding action is not performed
///              and the name will retain the same bits as before calling this function.
///              For new names, default is: non-public, non-weak, non-auto.
/// \retval 1  ok, name is changed
/// \retval 0  failure, a warning is displayed

idaman bool ida_export set_name(ea_t ea, const char *name, int flags=0);

/// \defgroup SN_ Set name flags
/// Passed as 'flag' parameter to set_name(ea_t, const char *, int)
//@{
#define SN_CHECK        0x00
#define SN_NOCHECK      0x01    ///< Don't fail if the name contains invalid characters.
                                ///< If this bit is clear, all invalid chars
                                ///< (those !is_ident_cp()) will be replaced
                                ///< by SUBSTCHAR
                                ///< List of valid characters is defined in ida.cfg
#define SN_PUBLIC       0x02    ///< if set, make name public
#define SN_NON_PUBLIC   0x04    ///< if set, make name non-public
#define SN_WEAK         0x08    ///< if set, make name weak
#define SN_NON_WEAK     0x10    ///< if set, make name non-weak
#define SN_AUTO         0x20    ///< if set, make name autogenerated
#define SN_NON_AUTO     0x40    ///< if set, make name non-autogenerated
#define SN_NOLIST       0x80    ///< if set, exclude name from the list.
                                ///< if not set, then include the name into
                                ///< the list (however, if other bits are set,
                                ///< the name might be immediately excluded
                                ///< from the list).
#define SN_NOWARN       0x100   ///< don't display a warning if failed
#define SN_LOCAL        0x200   ///< create local name. a function should exist.
                                ///< local names can't be public or weak.
                                ///< also they are not included into the list of names
                                ///< they can't have dummy prefixes.
#define SN_IDBENC       0x400   ///< the name is given in the IDB encoding;
                                ///< non-ASCII bytes will be decoded accordingly.
                                ///< Specifying SN_IDBENC also implies SN_NODUMMY
#define SN_FORCE        0x800   ///< if the specified name is already present
                                ///< in the database, try variations with a
                                ///< numerical suffix like "_123"
#define SN_NODUMMY      0x1000  ///< automatically prepend the name with '_' if it
                                ///< begins with a dummy suffix such as 'sub_'.
                                ///< See also SN_IDBENC
#define SN_DELTAIL      0x2000  ///< if name cannot be set because of a tail byte,
                                ///< delete the hindering item
//@}

inline bool force_name(ea_t ea, const char *name, int flags=0)
{
  return set_name(ea, name, flags|SN_FORCE|SN_NODUMMY);
}

/// \name Delete a name of a program item
/// \param ea  linear address
/// \retval 1  ok, name is deleted
/// \retval 0  failure, invalid address
//@{
inline bool del_global_name(ea_t ea) { return set_name(ea,"", SN_NOWARN); }
inline bool del_local_name(ea_t ea)  { return set_name(ea,"", SN_LOCAL|SN_NOWARN); }
//@}

/// Give an autogenerated (dummy) name.
/// Autogenerated names have special prefixes (loc_...).
/// \param from  linear address of the operand which references to the address
/// \param ea    linear address
/// \retval 1  ok, dummy name is generated or the byte already had a name
/// \retval 0  failure, invalid address or tail byte

idaman bool ida_export set_dummy_name(ea_t from, ea_t ea); // give dummy name


/// \name Set/Clear bit in flags for 'autogenerated but meaningful name'
/// This bit affects value of has_user_name(), has_auto_name() functions.
/// \param ea  linear address
/// \retval 1  ok
/// \retval 0  no meaningful name is present at the specified address
//@{
idaman bool ida_export make_name_auto(ea_t ea);
idaman bool ida_export make_name_user(ea_t ea);
//@}


enum ucdr_kind_t
{
  UCDR_STRLIT  = 0x01,   // string literals
  UCDR_NAME    = 0x02,   // regular (unmangled) names
  UCDR_MANGLED = 0x04,   // mangled names
  UCDR_TYPE    = 0x08,   // type names
};

enum nametype_t
{
  // identifier (e.g., function name)
  VNT_IDENT = UCDR_NAME|UCDR_MANGLED,
  // type name (can contain '<', '>', ...)
  VNT_TYPE = UCDR_TYPE,
  // UDT (structure, union, enum) member
  VNT_UDTMEM = UCDR_NAME,
  // string literal
  VNT_STRLIT = UCDR_STRLIT,
  VNT_VISIBLE = VNT_UDTMEM,// visible cp (obsolete; will be deleted)
};

/// Validate a name.
/// This function replaces all invalid characters in the name with SUBSTCHAR.
/// However, it will return false if name is valid but not allowed to be an
/// identifier (is a register name).
///
/// \param[in,out] name ptr to name. the name will be modified
/// \param         type the type of name we want to validate
/// \param         flags see SN_* . Only SN_IDBENC is currently considered
///
/// \return success

idaman bool ida_export validate_name(
        qstring *name,
        nametype_t type,
        int flags = 0);


/// Is the given codepoint acceptable in the given context?

idaman bool ida_export is_valid_cp(wchar32_t cp, nametype_t kind, void *data=NULL);


/// Mark the given codepoint (or range) as acceptable or unacceptable in the given context
/// If 'endcp' is not BADCP, it is considered to be the end of the range:
/// [cp, endcp), and is not included in the range

idaman void ida_export set_cp_validity(ucdr_kind_t kind, wchar32_t cp, wchar32_t endcp=BADCP, bool valid=true);


/// Is the given codepoint (or range) acceptable in the given context?
/// If 'endcp' is not BADCP, it is considered to be the end of the range:
/// [cp, endcp), and is not included in the range

idaman bool ida_export get_cp_validity(ucdr_kind_t kind, wchar32_t cp, wchar32_t endcp=BADCP);


/// Can a character appear in a name? (present in ::NameChars or ::MangleChars)

inline bool is_ident_cp(wchar32_t cp) { return is_valid_cp(cp, VNT_IDENT); }


/// Can a character appear in a string literal (present in ::StrlitChars)
/// If 'specific_ranges' are specified, those will be used instead of
/// the ones corresponding to the current culture (only if ::StrlitChars
/// is configured to use the current culture)

inline bool is_strlit_cp(wchar32_t cp, const rangeset_crefvec_t *specific_ranges=NULL)
{ return is_valid_cp(cp, VNT_STRLIT, (void *) specific_ranges); }


/// Can a character be displayed in a name? (present in ::NameChars)

inline bool is_visible_cp(wchar32_t cp)
{ return is_valid_cp(cp, VNT_VISIBLE); }


/// Is a valid name? (including ::MangleChars)

idaman bool ida_export is_ident(const char *name);


/// Is valid user-specified name? (valid name & !dummy prefix).
/// \param name  name to test. may be NULL.
/// \retval 1  yes
/// \retval 0  no

idaman bool ida_export is_uname(const char *name);


/// Is valid type name?
/// \param name  name to test. may be NULL.
/// \retval 1  yes
/// \retval 0  no

idaman bool ida_export is_valid_typename(const char *name);


/// Is dummy name?
/// \param name  name to test. may be NULL.
/// \return #BADADDR if not, otherwise the address denoted by the name

idaman ea_t ida_export dummy_name_ea(const char *name);


/// Extract a name or address from the specified string.
/// \param[out] out  output buffer for the identifier
/// \param line      input string
/// \param x         x coordinate of cursor
/// \return -1 if cannot extract. otherwise length of the name

idaman ssize_t ida_export extract_name(qstring *out, const char *line, int x);


/// Remove name from the list of names
/// \param ea  address of the name

idaman void ida_export hide_name(ea_t ea);


/// Insert name to the list of names

idaman void ida_export show_name(ea_t ea);


/// Get address of the name.
/// Dummy names (like byte_xxxx where xxxx are hex digits) are parsed by this
/// function to obtain the address. The database is not consulted for them.
/// This function works only with regular names.
/// \param from  linear address where the name is used.
///              if not applicable, then should be #BADADDR.
/// \param name  any name in the program or NULL
/// \return address of the name or #BADADDR

idaman ea_t ida_export get_name_ea(ea_t from, const char *name);


/// Get address of the name used in the expression for the address
/// \param from  address of the operand which references to the address
/// \param to    the referenced address
/// \return address of the name used to represent the operand

idaman ea_t ida_export get_name_base_ea(ea_t from, ea_t to);


/// Get value of the name.
/// This function knows about: regular names, enums, special segments, etc.
/// \param[out] value  pointer to variable with answer
/// \param from        linear address where the name is used
///                    if not applicable, then should be BADADDR
/// \param name        any name in the program or NULL
/// \return \ref NT_

idaman int ida_export get_name_value(uval_t *value, ea_t from, const char *name);

/// \defgroup NT_ Name value result codes
/// Return values for get_name_value()
//@{
#define NT_NONE         0       ///< name doesn't exist or has no value
#define NT_BYTE         1       ///< name is byte name (regular name)
#define NT_LOCAL        2       ///< name is local label
#define NT_STKVAR       3       ///< name is stack variable name
#define NT_ENUM         4       ///< name is symbolic constant
#define NT_ABS          5       ///< name is absolute symbol (#SEG_ABSSYM)
#define NT_SEG          6       ///< name is segment or segment register name
#define NT_STROFF       7       ///< name is structure member
#define NT_BMASK        8       ///< name is a bit group mask name
#define NT_REGVAR       9       ///< name is a renamed register (*value is idx into pfn->regvars)
//@}


/// Additional information for get_ea_name() function
struct getname_info_t
{
  size_t cb;            ///< size of this struct
  int32 inhibitor;      ///< codes to inhibit parts of demangled name (see \ref MNG_).
                        ///< Usually this is one of \inf{short_demnames} or \inf{long_demnames}.
  int32 demform;        ///< demangle only if \inf{demnames} is equal to 'demform'.
  int32 demcode;         ///< out: return value of demangler
  getname_info_t(void) : cb(sizeof(*this)), inhibitor(0), demform(0), demcode(0) {}
};


/// Get name at the specified address.
/// \param[out] out  buffer to hold the name
/// \param ea        linear address
/// \param gtn_flags how exactly the name should be retrieved.
///                  combination of \ref GN_ bits
/// \param gtni      additional information for name demangling
/// Please use the convenience functions declared below instead of calling
/// get_ea_name directly.
/// \return success

idaman ssize_t ida_export get_ea_name(
        qstring *out,
        ea_t ea,
        int gtn_flags=0,
        getname_info_t *gtni=NULL);

/// \defgroup GN_ bits for get_ea_name() function. There is a convenience
///           function calc_gtn_flags() to calculate the GN_LOCAL flag
//@{
#define GN_VISIBLE   0x0001 ///< replace forbidden characters by SUBSTCHAR
#define GN_COLORED   0x0002 ///< return colored name
#define GN_DEMANGLED 0x0004 ///< return demangled name
#define GN_STRICT    0x0008 ///< fail if cannot demangle
#define GN_SHORT     0x0010 ///< use short form of demangled name
#define GN_LONG      0x0020 ///< use long form of demangled name
#define GN_LOCAL     0x0040 ///< try to get local name first; if failed, get global
#define GN_ISRET     0x0080 ///< for dummy names: use retloc
#define GN_NOT_ISRET 0x0100 ///< for dummy names: do not use retloc
#define GN_NOT_DUMMY 0x0200 ///< do not return a dummy name

//@}

// Convenience functions for get_ea_name returning ssize_t

inline ssize_t get_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, gtn_flags);
}

inline ssize_t idaapi get_visible_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|gtn_flags);
}

inline ssize_t idaapi get_colored_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|GN_COLORED|gtn_flags);
}

inline ssize_t idaapi get_short_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|GN_DEMANGLED|GN_SHORT|gtn_flags);
}

inline ssize_t idaapi get_long_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|GN_DEMANGLED|GN_LONG|gtn_flags);
}

inline ssize_t idaapi get_colored_short_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|GN_COLORED|GN_DEMANGLED|GN_SHORT|gtn_flags);
}

inline ssize_t idaapi get_colored_long_name(qstring *out, ea_t ea, int gtn_flags=0)
{
  return get_ea_name(out, ea, GN_VISIBLE|GN_COLORED|GN_DEMANGLED|GN_LONG|gtn_flags);
}

inline ssize_t idaapi get_demangled_name(
        qstring *out,
        ea_t ea,
        int32 inhibitor,
        int demform,
        int gtn_flags=0)
{
  getname_info_t gtni;
  gtni.inhibitor = inhibitor;
  gtni.demform = demform;
  gtn_flags |= GN_VISIBLE | GN_DEMANGLED;
  return get_ea_name(out, ea, gtn_flags, &gtni);
}

inline ssize_t idaapi get_colored_demangled_name(
        qstring *out,
        ea_t ea,
        int32 inhibitor,
        int demform,
        int gtn_flags=0)
{
  return get_demangled_name(out, ea, inhibitor, demform, gtn_flags|GN_COLORED);
}

// Convenience functions for get_ea_name returning qstring

inline qstring get_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, gtn_flags);
  return out;
}

inline qstring get_visible_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|gtn_flags);
  return out;
}

inline qstring idaapi get_colored_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|GN_COLORED|gtn_flags);
  return out;
}

inline qstring idaapi get_short_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|GN_DEMANGLED|GN_SHORT|gtn_flags);
  return out;
}

inline qstring idaapi get_long_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|GN_DEMANGLED|GN_LONG|gtn_flags);
  return out;
}

inline qstring idaapi get_colored_short_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|GN_COLORED|GN_DEMANGLED|GN_SHORT|gtn_flags);
  return out;
}

inline qstring idaapi get_colored_long_name(ea_t ea, int gtn_flags=0)
{
  qstring out;
  get_ea_name(&out, ea, GN_VISIBLE|GN_COLORED|GN_DEMANGLED|GN_LONG|gtn_flags);
  return out;
}

inline qstring idaapi get_demangled_name(
        ea_t ea,
        int32 inhibitor,
        int demform,
        int gtn_flags=0)
{
  qstring out;
  getname_info_t gtni;
  gtni.inhibitor = inhibitor;
  gtni.demform = demform;
  gtn_flags |= GN_VISIBLE | GN_DEMANGLED;
  get_ea_name(&out, ea, gtn_flags, &gtni);
  return out;
}

inline qstring idaapi get_colored_demangled_name(
        ea_t ea,
        int32 inhibitor,
        int demform,
        int gtn_flags=0)
{
  qstring out;
  get_demangled_name(&out, ea, inhibitor, demform, gtn_flags|GN_COLORED);
  return out;
}

/// Calculate flags for get_ea_name() function
#ifdef FUNCS_HPP
inline int calc_gtn_flags(ea_t from, ea_t ea)
{
  return func_contains(get_func(from), ea) ? GN_LOCAL : 0;
}
#endif

/// Get name color.
/// \param from  linear address where the name is used.
///              if not applicable, then should be #BADADDR.
///              The kernel returns a local name color if the reference is
///              within a function, i.e. 'from' and 'ea' belong to the same function.
/// \param ea    linear address

idaman color_t ida_export get_name_color(ea_t from, ea_t ea);


/// \defgroup GETN_ Name expression flags
/// Passed as 'flags' parameter to get_name_expr()
//@{
#define GETN_APPZERO  0x0001  ///< meaningful only if the name refers to a structure.
                              ///< append a struct field name if the field offset is zero?
#define GETN_NOFIXUP  0x0002  ///< ignore the fixup information when producing the name
#define GETN_NODUMMY  0x0004  ///< do not create a new dummy name but pretend it exists
//@}

/// Convert address to name expression (name with a displacement).
/// This function takes into account fixup information and returns
/// a colored name expression (in the form  <name> +/- <offset>).
/// It also knows about structure members and arrays.
/// If the specified address doesn't have a name, a dummy name is generated.
/// \param[out] out   output buffer for the name
/// \param from       linear address of instruction operand or data referring to
///                   the name. This address will be used to get fixup information,
///                   so it should point to exact position of the operand in the
///                   instruction.
/// \param n          number of referencing operand. for data items specify 0
/// \param ea         address to convert to name expression
/// \param off        the value of name expression. this parameter is used only to
///                   check that the name expression will have the wanted value.
///                   'off' may be equal to BADADDR but this is discouraged
///                   because it prohibits checks.
/// \param flags      \ref GETN_
/// \return < 0 if address is not valid, no segment or other failure.
///          otherwise the length of the name expression in characters.

idaman ssize_t ida_export get_name_expr(
        qstring *out,
        ea_t from,
        int n,
        ea_t ea,
        uval_t off,
        int flags=GETN_APPZERO);

/// Get a nice colored name at the specified address.
/// Ex:
///   - segment:sub+offset
///   - segment:sub:local_label
///   - segment:label
///   - segment:address
///   - segment:address+offset
/// \param[out] buf  buffer to hold the name
/// \param ea        linear address
/// \param flags     \ref GNCN_
/// \return the length of the generated name in bytes.

idaman ssize_t ida_export get_nice_colored_name(
        qstring *buf,
        ea_t ea,
        int flags=0);

/// \defgroup GNCN_ Nice colored name flags
/// Passed as 'flags' parameter to get_nice_colored_name()
//@{
#define GNCN_NOSEG    0x0001 ///< ignore the segment prefix when producing the name
#define GNCN_NOCOLOR  0x0002 ///< generate an uncolored name
#define GNCN_NOLABEL  0x0004 ///< don't generate labels
#define GNCN_NOFUNC   0x0008 ///< don't generate funcname+... expressions
#define GNCN_SEG_FUNC 0x0010 ///< generate both segment and function names (default is to omit segment name if a function name is present)
#define GNCN_SEGNUM   0x0020 ///< segment part is displayed as a hex number
#define GNCN_REQFUNC  0x0040 ///< return 0 if the address does not belong to a function
#define GNCN_REQNAME  0x0080 ///< return 0 if the address can only be represented as a hex number
#define GNCN_NODBGNM  0x0100 ///< don't use debug names
#define GNCN_PREFDBG  0x0200 ///< if using debug names, prefer debug names over function names
//@}


/// Append names of struct fields to a name if the name is a struct name.
/// \param out      pointer to the output buffer
/// \param disp     displacement from the name
/// \param n        number of operand n which the name appears
/// \param path     path in the struct. path is an array of id's.
///                 maximal length of array is #MAXSTRUCPATH.
///                 the first element of the array is the structure id.
///                 consecutive elements are id's of used union members (if any).
/// \param plen     size of path array
/// \param flags    the input flags. they will be returned if the struct
///                 cannot be found.
/// \param delta    delta to add to displacement
/// \param appzero  should append a struct field name if the displacement is zero?
/// \return flags of the innermost struct member or the input flags

idaman flags_t ida_export append_struct_fields(
        qstring *out,
        adiff_t *disp,
        int n,
        const tid_t *path,
        int plen,
        flags_t flags,
        adiff_t delta,
        bool appzero);


/// Get offset within a structure if the operand refers to structure.
/// Ex:
///   \v{mov ax, somedata.field5-2 (before it was max ax, 3)}
/// for this instruction, op #1 the function will return
///   - disp: the value of 'field5', i.e. 5
///   - delta: -2
///   - path: the existing path if any
/// \param disp   pointer to displacement (answer will be here)
/// \param delta  pointer to displacement delta (answer will be here)
/// \param path   existing strpath (if any)
/// \param ea     linear address of instruction/data
/// \param n      number of operand
/// \return if success, then length of path + 1.
///          if failed, then 0.

idaman int ida_export get_struct_operand(
        adiff_t *disp,
        adiff_t *delta,
        tid_t *path,
        ea_t ea,
        int n);


/// \name Work with publicness of a name
//@{
idaman bool ida_export is_public_name(ea_t ea);
idaman void ida_export make_name_public(ea_t ea);
idaman void ida_export make_name_non_public(ea_t ea);
//@}


/// \name Work with weak names.
//@{
idaman bool ida_export is_weak_name(ea_t ea);
idaman void ida_export make_name_weak(ea_t ea);
idaman void ida_export make_name_non_weak(ea_t ea);
//@}

/// \name Work with the list of names
//@{

/// Get number of names in the list

idaman size_t ida_export get_nlist_size(void);

/// Get index of the name in the list
/// \warning returns the closest match.
///         may return idx >= size.

idaman size_t ida_export get_nlist_idx(ea_t ea);

/// Is name included into names list?

idaman bool ida_export is_in_nlist(ea_t ea);

/// Get address from the list at 'idx'

idaman ea_t ida_export get_nlist_ea(size_t idx);

/// Get name using idx

idaman const char *ida_export get_nlist_name(size_t idx);

/// Rebuild names list

idaman void ida_export rebuild_nlist(void);
//@}

/// Renumber dummy names

idaman void ida_export reorder_dummy_names(void);

/// Specify strategy for retrieving debug names
enum debug_name_how_t
{
  DEBNAME_EXACT,        ///< find a name at exactly the specified address
  DEBNAME_LOWER,        ///< find a name with the address >= the specified address
  DEBNAME_UPPER,        ///< find a name with the address >  the specified address
  DEBNAME_NICE,         ///< find a name with the address <= the specified address
};

/// ea, name pair
struct ea_name_t
{
  ea_t ea;
  qstring name;
  ea_name_t(void) : ea(BADADDR) {}
  ea_name_t(ea_t _ea, const qstring &_name) : ea(_ea), name(_name) {}
};
DECLARE_TYPE_AS_MOVABLE(ea_name_t);
typedef qvector<ea_name_t> ea_name_vec_t; ///< vector of ea,name pairs

/// \name Debug names
/// Debug names exist during the debugging session.
/// The kernel does not verify them for anything and happily accepts
/// any string as a name.
//@{
idaman int  ida_export set_debug_names(const ea_t *addrs, const char *const *names, int qty);
idaman bool ida_export set_debug_name(ea_t ea, const char *name);
idaman ssize_t ida_export get_debug_name(
        qstring *out,
        ea_t *ea_ptr,
        debug_name_how_t how);
idaman void ida_export del_debug_names(ea_t ea1, ea_t ea2);
idaman ea_t ida_export get_debug_name_ea(const char *name);
idaman void ida_export get_debug_names(ea_name_vec_t *names, ea_t ea1, ea_t ea2);
//@}


enum demreq_type_t
{
  DQT_NPURGED_8 = -8, // only calculate number of purged bytes (sizeof(arg)==8)
  DQT_NPURGED_4 = -4, // only calculate number of purged bytes (sizeof(arg)==4)
  DQT_NPURGED_2 = -2, // only calculate number of purged bytes (sizeof(arg)==2)
  DQT_COMPILER  = 0,  // only detect compiler that generated the name
  DQT_NAME_TYPE = 1,  // only detect the name type (data/code)
  DQT_FULL      = 2,  // really demangle
};

/// Demangle a name.
/// \param out   output buffer
/// \param name  name to demangle
/// \param disable_mask bits to inhibit parts of demangled name (see \ref MNG_).
///                     by the M_COMPILER bits a specific compiler can be
///                     selected (see \ref MT_).
/// \return ME_... or MT__ bitmasks from demangle.hpp

idaman int32 ida_export demangle_name(
        qstring *out,
        const char *name,
        uint32 disable_mask,
        demreq_type_t demreq=DQT_FULL);

/// Demangle a name.
inline qstring idaapi demangle_name(
        const char *name,
        uint32 disable_mask,
        demreq_type_t demreq=DQT_FULL)
{
  qstring out;
  demangle_name(&out, name, disable_mask, demreq);
  return out;
}


inline int32 detect_compiler_using_demangler(const char *name)
{
  return demangle_name(NULL, name, 0, DQT_COMPILER);
}

/// What name types to ignore
typedef int ignore_name_def_t;
const ignore_name_def_t
  ignore_none   = 0,
  ignore_regvar = 1,
  ignore_llabel = 2,
  ignore_stkvar = 3,
  ignore_glabel = 4;

/// Is the name defined locally in the specified function?
/// \param pfn    pointer to function
/// \param name   name to check
/// \param ignore_name_def  which names to ignore when checking
/// \param ea1    the starting address of the range inside the function (optional)
/// \param ea2    the ending address of the range inside the function (optional)
/// \return true if the name has been defined
idaman bool ida_export is_name_defined_locally(
        func_t *pfn,
        const char *name,
        ignore_name_def_t ignore_name_def,
        ea_t ea1=BADADDR,
        ea_t ea2=BADADDR);

// Clean a name.
// This function removes punctuation marks (underscores and dots) from both
// ends of the name, and other typical prefixes/suffixes. Name is assumed to
// have the following format: [j_][@][.][_*][imp_]name[@digits][_NN]
// \param out     output buffer
// \param ea      address of the name (used to remove the module name)
//                if != BADADDR, the optional prefix (module name) will be
//                removed
// \param name    name to clean
// \param flags   combination of CN_... bits
// \return true if returned a non-empty name
idaman bool ida_export cleanup_name(
        qstring *out,
        ea_t ea,
        const char *name,
        uint32 flags=0);

#define CN_KEEP_TRAILING__DIGITS 0x01 // do not remove "_\d+" at the end of name


#endif // _NAME_HPP
