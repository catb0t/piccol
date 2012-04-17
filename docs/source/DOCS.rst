
.. highlight:: html

.. role:: raw-html(raw)
   :format: html


*******************
The Piccol tutorial
*******************

What follows is an informal but complete documentation on how to write Piccol programs.

Syntax
======

.. _lexical-structure:

Lexical structure
-----------------

Piccol has three kinds of identifiers:

**Type identifiers**

    Typenames *must* start with a capital letter, otherwise it is a syntax error!
    Capital letters are the ASCII characters ``[A-Z]``. (No Unicode support yet.)

    Regex: ``[A-Z][A-Za-z0-9_]*``

    Examples: 
      * ``Int``
      * ``FooBar``
      * ``Type_1``

**Name identifiers** 

    Names *must* start with a lowercase letter.
    (Otherwise you get a syntax error, again.) 

    Regex: ``[a-z][A-Za-z0-9_]*`` 

    Examples:
      * ``foo``
      * ``aFunc``
      * ``a_11``

**Variable identifiers**

    Variables are like name identifiers, except they start with a
    backslash, ``'\'``. 

    Also, the special token ``\\`` (two backslashes) is parsed as a special variable identifier.

    Examples:
      * ``\foo``
      * ``\x_AB``
      * ``\\``

This is the complete list of all **tokens** that are used for parsing Piccol:

  ``\`` ``{`` ``}`` ``[`` ``]`` ``(`` ``)`` ``->`` ``:-`` ``=`` ``?`` ``;`` ``.`` ``/*`` ``*/`` ``#``

This is the complete list of **reserved words** used in Piccol:

  ``def`` ``nil`` ``false`` ``true``

These tokens are used in Piccol's **macro preprocessor**. They are also reserved, though not
strictly part of Piccol proper.

  ``<:`` ``:>`` ``<::`` ``::>``

Whitespace is not significant, except as a delimiter.

Comments
--------

Piccol has two types of comments: C-style ``/* ... */`` and script-style ``# ...``

Comments are parsed as whitespace.

.. _literal-values:

Literal values
--------------

Piccol **literals** can be of four primitive types:
  
  * Real numbers. ("Doubles".)
  * Signed integers.
  * Unsigned integers.
  * Booleans.
  * Symbolic constants. (Like Lisp 'symbols' or Prolog 'atoms'. Piccol calls them 'symbols'.)

**Real numbers** can be of two forms:

  1. An optional ``'-'``, followed by a string of digits, followed by a ``'.'``, followed by a string of digits.
  (Note that the second 'string of digits' is required; you must write ``1.0`` instead of ``1.``)

    Regex: ``-?[0-9]+\.[0-9]+``

    Example: ``-1.0``
 
  2. An optional ``'-'``, followed by a string of digits, followed by the character ``'f'``.
  (Note: no space between the digits and the ``'f'``!)

    Regex: ``-?[0-9]+f``

    Example: ``3f``

**Unsigned integers** have one form:
   
  A string of digits, followed by the mandatory character ``'u'``.
  (Note, again, no space between digits and ``'u'``.)

    Regex: ``[0-9]+u`` 

    Example: ``0u``

**Signed integers** have one form:

  An optional ``-``, followed by a string of digits.
    Regex: ``-?[0-9]+`` 

    .. note::

       A number without an ``'f'`` or ``'u'`` suffix is parsed as a *signed* integer!

    Example: ``123``

**Booleans** are either the token ``true`` or the token ``false``.

**Symbolic constants** have two forms:

  1. A string of arbitrary characters delimited by ``'``. (Single quotes.) 

     .. note::

       Only single quotes are supported. Double quotes are not used in Piccol.

  2. The special token ``nil``, which denotes a special invalid symbol that cannot be represented as text.

Characters inside a symbolic constant can be quoted using the backslash, ``'\'``:

  * ``\n`` will expand to the newline symbol, like a ``'\n'`` in C.
  * ``\t`` will expand to the tab symbol, like a ``'\t'`` in C.
  * Any other symbol that follows a backslash will be used as-is in the symbolic constant.
    Thus, ``\'`` way to include a single quote in a symbolic constant.

.. note::

  No other characters have special meaning in a symbolic constant.

Thus, you can freely use newlines, non-printable symbols, Unicode, the null symbol, etc. in a symbolic constant.

Examples of symbolic constants:

  * ``''``
    The empty string.
  * ``'foo'``
  * ``'hello\nworld'``
  * ``'tab\tseparated\ttext'``
  * ``'Single quotes are \'quoted\' too.'``
  * ``'oops: \032'``

    .. note::

      The backslash in this symbol is ignored! It is equivalent to ``'oops: 032'``.
  
Typenames
---------

A **typename** is either a type identifier, or a tuple name.

Type identifiers -- see :ref:`lexical-structure` above.

Examples: 
  * ``Int`` 
  * ``MyType``
  * ``Type_1``.

A **tuple name** is a space-separated list (possibly empty) of typenames between ``[`` and ``]``.

For example:

  * ``[ Int ]``
  * ``[]``
  * ``[ Real Real MyType ]``
  * ``[Int [Foo Bar] [Atype Btype]]``

Type definitions
----------------

A **type definition** starts with the keyword 'def' and ends in a semicolon, and has two forms: ::

  def [ <fields> ];

or ::

  def { <fields> } <typeIdentifier>;


Where <fields> is a (possibly empty) space-separated list of fields, where each field has the form ::

  <name> : <typename>

(Whitespace is optional.)

Here ``<name>`` is a name identifier, ``<typeIdentifier>`` is a type identifier (see :ref:`lexical-structure` above) and
``<typename>`` is a typename.

Examples of type definitions: ::

  def [ a:Int b:Int ];

  def { foo:FooType b:[Int Int] } Bar;

  def {} Void ;

  def { f1: [Int] 
        f2: [UInt] 
        f3: Data } AnotherType;

Function definitions
--------------------

A **function definition** looks like this: ::

  <name> <typename> -> <typename> :- <expression> .

Where ``<name>`` is a name identifier (see :ref:`lexical-structure` above), and ``<typename>`` is a typename.

(We'll get to ``<expression>`` later, but note that a function body starts with a ``:-`` token and ends with a ``.`` (period).)

Examples of functions: ::

  foobar Void->MyType :-
    MyType{a=1}
  .

  some_func [Int Int]->Int :- \\ add->Int.

  maketwoints UInt-> [ Int Int ] :- 
    [ (one->Int) (two->Int) ].

 
Expressions
-----------

Piccol has a postfix function call syntax, similar to stack-based languages like Forth and Factor.

An **expression** (remember that the body of a function is also an expression!) is a list of space-separated elements, where each element is one of:

  * A value
  * A function call
  * A lambda function call
  * A structure modifier
  * A control construct

You can think of Piccol functions as a kind of Unixy pipe: structures are pushed into the left side of the 
pipeline, and as you move right the original structure goes through function calls and is transformed.
There are also a few special control constructs to interrupt a pipeline or jump into the execution another one.

Examples of function bodies: ::

  1 add->UInt split->[UInt UInt] convert->Real print

  [ Foo{a=1} Bar{b=2} ] do_something->Foo ok->Bool ? 'OK' print ; 'Oops.' print

.. _values:

Values
------

A **value** is one of: 

  * A constructed tuple 
  * A constructed structure
  * A variable
  * A literal value

A **constructed tuple** looks like this: 

  ::

    [ <valuesOrParens> ]

  where ``<valuesOrParens>`` is a space-separated list of ``<valueOrParen>``, where each ``<valueOrParen>`` is either a 
    * value or an 
    * expression between ``(`` and ``)``.

  .. note::

    The parentheseses are required! If you want to assign an arbitrary expression to a field, you must enclose it
    in parentheses, otherwise the parser will get confused.

  Examples:
    * ``[1 2 3]``
    * ``[]``
    * ``['hello' 'world']``
    * ``[ \one \two ]``
    * ``[ (one->Int) (two->Int) (1 inc->Int) ]``

A **constructed structure** looks like this: 

  ::

    <typename> { <fieldlist> }

  Where ``<fieldlist>`` is a space-separated list of key-value pairs like this: ::

    <name> = <valueOrParen>

  Examples:
    * ``MyType {firstfield=1 secondfield=2}``
    * ``Foo{ a = [] b = Bar{a=1 b=2} }``
    * ``Void{}``
    * ``SubExpr{ val=([1u 2u] complex_calculation->Real) }``

A **variable** is a variable identifier, see :ref:`lexical-structure` above.

For **literal values** see :ref:`literal-values` above.

.. note::

  This means that you *cannot* use parenthesised expressions in a function definition, neither can you nest
  parens! Parens are only used for setting fields in structures, they are not needed in an expression.

.. note::

  The above explanation is not quite correct. Parenthesised expressions differ from 'normal' expressions in
  function and lambda bodies in that parenthesised expressions *cannot* have control constructs. But don't worry, this
  is natural, since using a control construct while setting a structure field value makes no sense.

.. _function-calls:

Function calls
--------------

A **function call** looks like this: ::

  <name> -> <typename>

or like this: ::

  <name>

The second form is simply syntactic sugar; it is equivalent in all ways to writing ``<name> -> Void``.

Examples:
  * ``foo->Bar``
  * ``run->Result``
  * ``print``

    Note: equivalent to ``print->Void``

.. note:: 

  There is also a special set of 'inline assembly' functions, which have a ``$`` as the first character of ``<name>``.
  (Example: ``$add``, ``$mul``)
  They are built into the language and cannot be defined by the programmer. They'll be explained later.

.. _lambda-function-calls:

Lambda function calls
---------------------

A **lambda function** in Piccol is an anonymous function that is defined in-line while an expression is parsed and 
immediately called.

It looks like this: ::

  -> <typename> ( <functionBody> )

A ``<functionBody>`` is an expression; anything you can put in the body of function that's defined at toplevel you
can also put in the body of a lambda function.

Examples:
  * ``->Void('uh-oh' print)``
  * ``-> Int ( \v )``
  * ``-> Void ( 1 try_this; 2 try_another; 'oops' print fail )``

.. _structure-modifiers:

Structure modifiers
-------------------

**Structure modifiers** look exactly like curly-brace structure constructors, except without the leading ``<typename>``: ::

  { <fieldlist> }

(See :ref:`values` above.)

Examples:
  * ``{a=1}``
  * ``{ x=(get_x->Int) y=(get_y->Int) }``

Control constructs
------------------

There are exactly three **control constructs**. All three are reserved tokens in the language.

  * ``?``
  * ``;``
  * ``fail``

Semantics
=========

Types
-----

Piccol types have several very restrictive properties:
  * They are always fixed-size. The size (in bytes) of a value of any given type is always constant at runtime.
  * Types cannot reference themselves recursively. 
  * Types cannot reference types that have not been defined yet.
  * There is no concept of a 'pointer' or 'reference' type. (All types are 'value types'.)

Piccol types are nominal, not structural. That means that if two types have different names they are considered
by the compiler to be two different types. (Even if they have exactly the same definition!)

Piccol has five basic, 'atomic' data types:

  * ``Int``, the signed 64-bit integer.
  * ``UInt``, the unsigned 64-bit integer.
  * ``Real``, the floating-point double.
  * ``Bool``, the boolean value. (Implemented internally as a 64-bit integer, 0 or 1.)
  * ``Sym``, the symbol. (Implemented internally as a 64-bit unsigned integer index into a symbol table.)
  * ``Void``, the empty type, which takes up no space and has no runtime representation.

They are equivalent to the following definitions, except for the fact that Piccol does not allow types to 
reference themselves. ::

  def { v:Int } Int;
  def { v:UInt } UInt;
  def { v:Real } Real;
  def { v:Bool } Bool;
  def { v:Sym } Sym;
  def { } Void;

There are two kinds of non-atomic composite types in Piccol: tuples and structures.

Internally, they are really the same. Tuple types are only a special kind of syntactic sugar so that
the programmer is not forced to invent meaningless typenames.

A tuple type definition looks like this: ::

  [ <typename>... ]

For example, ::

  [ Int MyType [Sym Sym ] ]

A definition of a tuple type is exactly equivalent to a corresponding structure type, so that ::

  def [ a:Int b:Int ];

is the exact same thing as ::

  def { a:Int b:Int } [ Int Int ];

Here the ``[ Int Int ]`` is treated as a type identifier by the compiler. 
(Even though it has spaces and brackets in the name!)

The same hold for using a tuple type, too: ::

  [ 12 89 'tag' ] foo->Int

is the exact same thing as ::

  [ Int Int Sym ] {a=12 b=89 c='tag} foo->Int

.. note::

  Tuple types *must* be defined before use, just like structure types!

Function evaluation
-------------------

Function names have three elements: 
  * the function name proper
  * the function's input type
  * the function's output type

Functions can be overloaded on any of the three elements; thus, two functions with the same name but
different input types are different and completely separate functions for the compiler.
(The same for functions that have the same name and the same input type, but different output types.)

A function can only have *one* input type and *one* output type.

.. note::

    If you want a function to accept several values, you will need to define a composite type. 
    Likewise, if you want a function that does not accept values or does not return them, use Void.

When a function is called it will either *succeed*, returning a type, or it will *fail*, not returning any value.

('Failure' is very similar to exceptions in other languages, except that Piccol failures don't have a type and
have a ligher implementation in the VM.)

When a function is called, (see :ref:`function-calls` above) Piccol's syntax puts the input value on the *left* of the 
function name, while the function's return type must be specified on the *right*.

.. note::

   If a return type isn't specified then ``Void`` is assumed. There is no type inference in Piccol.

Examples:
  * ``[1 2] add->Int``
  * ``Foo{a=1} add->Int square->Real``
  * ``'hello' print exit``
    (Equivalent to ``'hello' print->Void exit->Void``)

Lambda functions
----------------

(See :ref:`lambda-function-calls` above.)

A 'lambda function' is exactly the same as a 'normal' function, except that it 
  * doesn't have a name (it still has an input and return type, though!)
  * is defined inside the expression of another function or lambda function
  * is called at the same place where it is defined.

.. note::

  There is no concept of a function pointer or closure in Piccol. Thus, you can't return or pass a lambda function as
  an object. Lambda functions are only used as a way of structuring code, similar to curly-brace blocks in C.

Note that lambda functions will succeed or fail, just like 'normal' functions!

Structure modifiers
-------------------

(See :ref:`structure-modifiers` above.)

A structure modifier is really a special kind of syntactic sugar for changing fields in a structure.
It could be replaced by a lambda function, except that structure modifiers are implemented in a slightly
more efficient manner under the hood.

Here's an explanation by example:

Consider a structure like this: ::

  def { a:Int b:UInt c:Real } Foo;

Consider this code: ::

  Foo{ a=1 b=2u c=3f } { b=100u }

Then this code is fully equivalent to the following: ::

  Foo{ a= 1 b=2u c=3f } ->Foo( Foo{ a=\a b=100u c=\c } )

That is, the structure is accepted by the modifier, the fields specified in the modifier are changed, and 
the structure is then returned. (Without changing any other fields!)

Constructing structures
-----------------------

.. note::

  When constructing a structure, you don't have to specify all of its fields. The fields you leave out
  will be initialized with default values:

    * ``Int``: ``0``
    * ``UInt``: ``0u``
    * ``Real``: ``0f``
    * ``Bool``: ``false``
    * ``Sym``: ``nil``

For example: ::

  def {a:Int b:Sym c:Bool} Foo;

  Foo {}

Here the object that is constructed is really ``Foo { a=0 b=nil c=false }``

Variables
---------

'Variables' (however incorrectly names; really they should be called 'fieldrefs') are a way to access the data
of a function's input type.

(See 'variable identifiers' above in :ref:`lexical-structure`.)

There are two ways to access the function's input type:

  * By accessing the contents of a certain field of the input value. (By using the ``\<fieldname>`` syntax.)
  * By accessing the input value as a whole. (The special token ``\\``)

Examples:

Here ``myfunc`` accepts a ``MyType`` value and returns the contents of its field ``a``. ::

  def { bar:Int } Foo;
  def { a:Int b:Foo } MyType;

  myfunc MyType->Int :- \a.

Here the field ``b`` is returned instead: ::

  myfunc2 MyType->Foo :- \b.

Here ``func`` accepts a ``MyType`` value and passes it to another function, ``frob``, returning nothing. ::

  func MyType->Void :- \\ frob.

Field references work exactly the same for tuples as they do for structures: ::

  def [one:Int two:Int three:Int];

  foo [Int Int Int]->Int :- [ ([\one \two] add->Int) \three ] add->Int.

.. note::

  Piccol provides several predefined types: ::

     def [ a:Int  b:Int  ];
     def [ a:UInt b:UInt ];
     def [ a:Bool b:Bool ];
     def [ a:Real b:Real ];
     def [ a:Sym  b:Sym  ];

     def [ v:Int ];
     def [ v:UInt ];
     def [ v:Sym ];
     def [ v:Bool ];
     def [ v:Real ];

  Keep that in mind when reading Piccol code.

Control constructs
------------------

There are three control constructs in Piccol:

  * ``;``
  * ``fail``
  * ``?``

The body of a function (whether a 'normal' toplevel function or a lambda) can be segmented into several
so-called 'branches'.

Piccol has a simple runtime model: if, while evaluating a function body, any function fails, then the evaluation
of the current branch is aborted (discarding any intermediate results) and evaluation jumps to the next branch in list.
If all branches fail, then the function itself fails.
If a branch succeeds, then the functions successfully returns and the remaining branches are ignored.

Branches
--------

Branches are delimited by the ``;`` token. 

Examples: ::

  one_of_3 [Int Int Int]->Void :- 
     \a foo ;
     \b foo ;
     \c foo.

Here, ``one_of_3`` tries to call the function ``foo`` with one of three arguments, returning successfully if any of the
three variants succeeds. If all three attempts at calling ``foo`` fail, then ``one_of_3`` itself fails. ::

  pick_one [Sym Sym]->Int :- [ 'foo' (\\->Sym(\a foo->Sym; \b foo->Sym)) ] bar->Int.

This is a more complex example, demonstrating branches inside a lambda function.
Here the function ``pick_one`` accepts two symbol values, passing either one of them to ``bar``, 
depending on whether ``foo`` succeeds or not.
Note that if ``foo`` fails for both values, then ``pick_one`` itself will fail. 
(Failures are bubbled up through the evaluation stack.) ::

  tryit Something->Void :- \\ foo; .

In this example the function ``tryit`` attempts to call ``foo`` with an argument of type ``Something``; if ``foo`` fails, then
``tryit`` succeeds anyways.

.. note::

  The function could be rewritten as ::

     tryit Something->Void :- \\ foo->Void; Void{} .

  The compiler inserts proper ``Void{}`` constructs where necessary and allows you to omit ``->Void`` for functions
  that return ``Void``.

  Note, however, that when returning a real type you need to annotate types and return values properly.

Example: ::

  tryit Thing->Int :- \\ convert->Int; 42.

This version of ``tryit`` tries to convert a value of type ``Thing`` to an ``Int``, using the ``convert`` function; if ``convert``
fails, then ``tryit`` returns the default value of ``42``.

Tests and fails
---------------

The special token ``fail`` fails the current branch, unconditionally. It acts like a ``Void->Void`` function that always
fails.

It can be useful for implementing certain control structures. For example: ::

  try_all [Int Int Int]->Void :-
     \a foo fail ;
     \b foo fail ;
     \c foo fail ;
     .

Here ``try_all`` calls ``foo`` three times with three different ``Int`` arguments. ``try_all`` ignores failures of ``foo`` and
succeeds unconditionally.

The specials token ``?`` checks its boolean argument and succeeds or fails accordingly.
It acts like a ``Bool->Void`` function; its argument on the left must be a ``Bool``. If it is ``true``, then ``?`` succeds,
if it is ``false``, then ``?`` fails. There are no other effects.

Example: ::

  test Sym->Void :- \\ ok->Bool ? func_then; func_else.

This example shows how to implement a traditional 'if-then-else' control structure in Piccol.
If ``ok`` returns ``true``, then ``func_then`` will be called; if it returns ``false``, then ``func_else``.

Arithmetic and logic
====================

