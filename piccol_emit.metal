

sym :- @'PUSH' \any @'Sym' &''.

field_name :- 'FIELD_NAME' sym.

field :-  field_name 'FIELD_TYPE' 'Sym'  @'PUSH' @'Int' @'2' @'DEF_FIELD'.
field :-  field_name 'FIELD_TYPE' 'Int'  @'PUSH' @'Int' @'3' @'DEF_FIELD'.
field :-  field_name 'FIELD_TYPE' 'UInt' @'PUSH' @'Int' @'4' @'DEF_FIELD'.
field :-  field_name 'FIELD_TYPE' 'Real' @'PUSH' @'Int' @'5' @'DEF_FIELD'.
field :-  field_name 'FIELD_TYPE' sym    @'DEF_STRUCT_FIELD'.
field :-  field_name 'EMPTY_TYPE'        @'PUSH' @'Int' @'1' @'DEF_FIELD'.

fields :- field fields.
fields :- .

def :- 'START_DEF' @'_cmode_on' @'NEW_SHAPE' fields 'DEF_NAME' sym 'END_DEF' @'DEF_SHAPE' @'_cmode_off'.



val_literal :- \any &''.

primitive_type :- 'Int' &''.
primitive_type :- 'Real' &''.
primitive_type :- 'Bool' &''.
primitive_type :- 'Sym' &''.

primitive_type_x :- primitive_type &'push'.

val :- 'SET_TYPE' @'PUSH' primitive_type_x 'PUSH' val_literal @'_push_type' &'pop'.

val_primitive :- val 'CALL' '$cast'
                 @'PUSH' @'Sym' primitive_type 
                 @'PUSH' @'Sym' @'_pop_type' primitive_type_x @'SYSCALL_PRIMITIVE'.
val_primitive :- val.

val_or_call :- val_primitive.
val_or_call :- statements.

structfield :- val_or_call 
               'SELECT_FIELD' @'_fieldname_deref' val_literal
               'CHECK_TYPE' @'_fieldtype_check' val_literal @'_pop_type'
               'SET_FIELD' @'_type_size' @'SET_FIELDS'.

structfields :- structfield structfields.
structfields :- .


structval :- 'SET_TYPE' @'_push_type' val_literal 
             'START_STRUCT' @'_type_size' @'NEW_STRUCT' 
             structfields 
             'END_STRUCT'
             .

funcall :- 'CALL' 
           sym 
           @'_top_type' @'_pop_type' 
           @'_push_type' val_literal @'_top_type' 
           @'CALL'.


statements_x :- structval statements_x.
statements_x :- funcall statements_x.
statements_x :- .

statements :- @'_push_type' @'Void' funcall statements_x.
statements :- statements_x.

fun :- 'FUN_TYPE' @'_push_funlabel' val_literal val_literal val_literal
       'START_FUN' statements 
       'END_FUN' @'EXIT' @'_drop_types' @'_pop_funlabel'.


all :- def all.
all :- fun all.
all :- @'EXIT'.

main :- all.
