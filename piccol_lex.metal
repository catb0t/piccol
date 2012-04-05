
space :- ' '.
space :- '\n'.
space :- '\t'.

comment_x :- '\n'.
comment_x :- \any comment_x.
comment_xx :- '*/'.
comment_xx :- \any comment_xx.

comment :- '#' comment_x.
comment :- '/*' comment_xx.

spaces :- space spaces.
spaces :- comment spaces.
spaces :- .

any_letter :- \upcase.
any_letter :- \locase.

upcase_letter :- \upcase.
locase_letter :- \locase.

digit :- \digit.


ident_x :- any_letter ident_x.
ident_x :- digit ident_x.
ident_x :- '_' ident_x.
ident_x :- .

ident :- locase_letter ident_x.

ident_here :- ident &''.


typename :- upcase_letter ident_x.

tuplename_x :- @' ' &'combine' typename_here &'combine' spaces tuplename_x.
tuplename_x :- .

tuplename :- '[' &'append' spaces tuplename_x ']' @' ]' &'combine'.

typename_here :- typename &''.
typename_here :- &'push' tuplename &'pop'.

opttype :- spaces ':' spaces @'FIELD_TYPE' typename_here.
opttype :- spaces @'EMPTY_TYPE'.

structfields :- spaces @'FIELD_NAME' ident_here opttype structfields.
structfields :- spaces .

tuptype :- @' ' &'combine' typename_here &'combine_keep'.

tuplefields :- spaces @'FIELD_NAME' ident_here spaces ':' spaces @'FIELD_TYPE' tuptype tuplefields.
tuplefields :- spaces .

structdef :- '{' structfields '}' 
             spaces @'DEF_NAME' typename_here spaces ';' @'END_DEF'.

tupledef :- '[' &'push' tuplefields ']' @' ]' &'combine' spaces ';'
            spaces @'DEF_NAME' &'pop' @'END_DEF'.

defhead :- spaces 'def' @'START_DEF' spaces.

def :- defhead structdef.
def :- defhead tupledef.



uintval_x :- digit uintval_x.
uintval_x :- .
uintval :- digit uintval_x.

symval_x :- {\\} {'} @{'}  &'combine' symval_x.
symval_x :- {\\} {t} @{\t} &'combine' symval_x.
symval_x :- {\\} {n} @{\n} &'combine' symval_x.
symval_x :- {'} &'pop'.
symval_x :- \any &'append' symval_x.
symval :- &'push' {'} symval_x.

nilval :- 'nil'.
falseval :- 'false'.
trueval :- 'true'.


ident_struct_field :- ident &'push'.

structvalfields :- spaces ident_struct_field spaces '=' spaces val_or_call 
                   @'SELECT_FIELD' &'pop' @'SET_FIELD' 
                   structvalfields.

structvalfields :- spaces.

structval :- @'SET_TYPE' typename_here spaces '{' @'START_STRUCT' structvalfields '}' @'END_STRUCT'.


tuplevalfields :- spaces val_or_call @'SET_TUPLEFIELD' tuplevalfields.
tuplevalfields :- spaces.

tupleval :- spaces '[' @'START_TUPLE' tuplevalfields ']' @'END_TUPLE'.

val :- nilval  @'SET_TYPE' @'Sym' @'PUSH_RAW' @'0'.
val :- falseval @'SET_TYPE' @'Bool' @'PUSH' @'0'.
val :- trueval @'SET_TYPE' @'Bool' @'PUSH' @'1'.
val :- @'SET_TYPE' @'Sym'  @'PUSH' symval.


val_number_neg :- '-' uintval '.' uintval &'push' @'SET_TYPE' @'Real' @'PUSH' &'pop'.
val_number_neg :- '-' uintval &'push' 'f'         @'SET_TYPE' @'Real' @'PUSH' &'pop'.
val_number_neg :- '-' uintval &'push'             @'SET_TYPE' @'Int'  @'PUSH' &'pop'.

val_number :- uintval '.' uintval &'push' @'SET_TYPE' @'Real' @'PUSH' &'pop'.
val_number :- uintval &'push' 'f'         @'SET_TYPE' @'Real' @'PUSH' &'pop'.
val_number :- uintval &'push' 'u'         @'SET_TYPE' @'UInt' @'PUSH' &'pop'.
val_number :- uintval &'push'             @'SET_TYPE' @'Int'  @'PUSH' &'pop'.

val :- val_number_neg.
val :- val_number.

val_primitive :- val.

variable :- @'DEREF' '\\' ident_here.
variable :- '\\\\' @'DEREF_HEAD'.


val_or_call :- tupleval.
val_or_call :- structval.
val_or_call :- '(' spaces @'OPTIONAL_VOID' paren_statements.
val_or_call :- variable.
val_or_call :- val_primitive.


funcall :- '?' @'ASMCALL' @'if'.
funcall :- 'fail' @'ASMCALL' @'fail'.
funcall :- @'ASMCALL' '$' ident_here.
funcall :- @'CALL' ident_here spaces '->' spaces typename_here.
funcall :- @'CALL' ident_here @'Void'.

change_struct :- '{' @'CHANGE_STRUCT' structvalfields '}' @'END_CHANGE_STRUCT'.

statement :- tupleval spaces.
statement :- structval spaces.
statement :- variable spaces.
statement :- change_struct spaces.
statement :- lambda spaces.
statement :- val_primitive spaces.
statement :- funcall spaces.

paren_statements :- ')'.
paren_statements :- statement paren_statements.

lambda :- '->' spaces @'START_LAMBDA' typename_here spaces 
          '(' spaces @'START_BRANCH' @'OPTIONAL_VOID' lambda_statements.

lambda_statements :- ')' spaces @'END_BRANCH' @'END_LAMBDA'.
lambda_statements :- statement_or_branch lambda_statements.

statement_or_branch :- ';' @'END_BRANCH' @'START_BRANCH' @'OPTIONAL_VOID' spaces.
statement_or_branch :- statement.

toplevel_statements :- '.' @'END_BRANCH' .
toplevel_statements :- statement_or_branch toplevel_statements.

fun :- spaces @'FUN_TYPE' ident_here
       spaces typename_here spaces '->' 
       spaces typename_here 
       spaces ':-' 
       @'START_FUN' spaces 
       @'START_BRANCH' @'OPTIONAL_VOID' toplevel_statements 
       @'END_FUN'.


all :- def all.
all :- fun all.
all :- spaces.

prelude :- .

main :- prelude all.

