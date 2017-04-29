#ifndef NVIM_MARK_EXTENDED_H
#define NVIM_MARK_EXTENDED_H

#include "nvim/mark_extended_defs.h"
#include "nvim/lib/kbtree.h"
#include "nvim/lib/kvec.h"
#include "nvim/map.h"


// TODO(timeyyy): Support '.', normal vim marks etc
#define Extremity -1    // Start or End of lnum or col


#define extline_cmp(a, b) (kb_generic_cmp((a)->lnum, (b)->lnum))

// TODO(timeyyy): implement kb_itr_interval

// Macro Documentation: FOR_ALL_?
// Search exclusively using the range values given.
// -1 can be input for range values to the start and end of the buffer

// see FOR_ALL_ for documentation
#define FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = l_lnum == Extremity ? MINLNUM : l_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_next(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_next(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum > u_lnum && u_lnum != Extremity) { \
      break;\
    }\
      code;\
    }

// see FOR_ALL_ for documentation
#define FOR_ALL_EXTMARKLINES_PREV(buf, l_lnum, u_lnum, code)\
  kbitr_t(extlines) itr;\
  ExtMarkLine t;\
  t.lnum = u_lnum == Extremity ? MAXLNUM : u_lnum;\
  if (!kb_itr_get(extlines, &buf->b_extlines, &t, &itr)) { \
    kb_itr_prev(extlines, &buf->b_extlines, &itr);\
  }\
  ExtMarkLine *extline;\
  for (; kb_itr_valid(&itr); kb_itr_prev(extlines, &buf->b_extlines, &itr)) { \
    extline = kb_itr_key(&itr);\
    if (extline->lnum < l_lnum && l_lnum != Extremity) { \
      break;\
    }\
    code;\
  }

// see FOR_ALL_ for documentation
#define FOR_ALL_EXTMARKS(buf, ns, l_lnum, l_col, u_lnum, u_col, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.ns_id = ns;\
  mt.mark_id = 0;\
  mt.line = NULL;\
  mt.col = l_col ==  Extremity ? MINCOL : l_col;\
  FOR_ALL_EXTMARKLINES(buf, l_lnum, u_lnum, { \
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_next(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; kb_itr_valid(&mitr); kb_itr_next(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (extmark->col > u_col && u_col != Extremity) { \
        break;\
      }\
      code;\
    }\
  })


// see FOR_ALL_ for documentation
#define FOR_ALL_EXTMARKS_PREV(buf, ns, l_lnum, l_col, u_lnum, u_col, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.mark_id = sizeof(uint64_t);\
  mt.ns_id = ns;\
  mt.col = u_col == Extremity ? MAXCOL : u_col;\
  FOR_ALL_EXTMARKLINES_PREV(buf, l_lnum, u_lnum, { \
    if (!kb_itr_get(markitems, &extline->items, mt, &mitr)) { \
        kb_itr_prev(markitems, &extline->items, &mitr);\
    } \
    ExtendedMark *extmark;\
    for (; kb_itr_valid(&mitr); kb_itr_prev(markitems, &extline->items, &mitr)) { \
      extmark = &kb_itr_key(&mitr);\
      if (extmark->col < l_col && l_col != Extremity) { \
          break;\
      }\
      code;\
    }\
  })


#define FOR_ALL_EXTMARKS_IN_LINE(items, code)\
  kbitr_t(markitems) mitr;\
  ExtendedMark mt;\
  mt.ns_id = 0;\
  mt.mark_id = 0;\
  mt.line = NULL;\
  mt.col = 0;\
  if (!kb_itr_get(markitems, items, mt, &mitr)) { \
    kb_itr_next(markitems, items, &mitr);\
  } \
  ExtendedMark *extmark;\
  for (; kb_itr_valid(&mitr); kb_itr_next(markitems, items, &mitr)) { \
    extmark = &kb_itr_key(&mitr);\
    code;\
  }\


int mark_cmp(ExtendedMark a, ExtendedMark b);

#define markitems_cmp(a, b) (mark_cmp((a), (b)))
KBTREE_INIT(markitems, ExtendedMark, markitems_cmp, 10)


typedef struct ExtMarkLine
{
  linenr_T lnum;
  kbtree_t(markitems) items;
} ExtMarkLine;


KBTREE_INIT(extlines, ExtMarkLine *, extline_cmp, 10)


extern uint64_t extmark_namespace_counter;

// All extmark namespaces that exist in nvim
typedef Map(uint64_t, cstr_t) _NS;
_NS *EXTMARK_NAMESPACES;


typedef PMap(uint64_t) IntMap;


typedef struct ExtmarkNs {  // For namespacing extmarks
  IntMap *map;              // For fast lookup
  uint64_t free_id;         // For automatically assigning id's
} ExtmarkNs;


typedef kvec_t(ExtendedMark *) ExtmarkArray;


// Undo/redo extmarks

typedef enum {
  kExtmarkNOOP,      // Extmarks shouldn't be moved
  kExtmarkNoReverse, // Iterate the undo/redo in LIFO order
  kExtmarkNoUndo,    // The move should not be reversable
  kExtmarkReverse,   // Start marker for FIFO iteration of the undo/redo items
  kExtmarkReverseEnd, // Stop marker for FIFO iteration of the undo/redo items
} ExtmarkReverse;


typedef struct {
  linenr_T line1;
  linenr_T line2;
  long amount;
  long amount_after;
} Adjust;

typedef struct {
  linenr_T lnum;
  colnr_T mincol;
  long col_amount;
  long lnum_amount;
} ColAdjust;

typedef struct {
  linenr_T line1;
  linenr_T line2;
  linenr_T last_line;
  linenr_T dest;
  linenr_T num_lines;
  linenr_T extra;
} AdjustMove;

typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  linenr_T lnum;
  colnr_T col;
} ExtmarkSet;

typedef struct {
  uint64_t ns_id;
  uint64_t mark_id;
  linenr_T old_lnum;
  colnr_T old_col;
  linenr_T lnum;
  colnr_T col;
} ExtmarkUpdate;

typedef struct undo_object ExtmarkUndoObject;

typedef struct {
  ExtmarkUndoObject *items;
  size_t size, capacity;
} ExtmarkUndoArray;

typedef enum {
  kAdjust,  // TODO(timeyyy): rename all references to adjust to LineAdjust?
  kColAdjust,
  kAdjustMove,
  kExtmarkSet,
  kExtmarkUnset,
  kExtmarkUpdate,
} UndoObjectType;

struct undo_object {
  UndoObjectType type;
  ExtmarkReverse reverse;
  union {
    Adjust adjust;
    ColAdjust col_adjust;
    AdjustMove move;
    ExtmarkSet set;
    ExtmarkUpdate update;
  } data;
};

typedef kvec_t(ExtmarkUndoObject) extmark_undo_vec_t;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "mark_extended.h.generated.h"
#endif

#endif  // NVIM_MARK_EXTENDED_H
