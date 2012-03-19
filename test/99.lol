
def [ a:Int b:Int ];
def [ a:Bool b:Bool ];
def [ a:Real b:Real ];

def [ a:Int b:Sym ];
def [ a:Int b:Bool ];

def [a:Int b:Sym c:Sym];
def [a:[Int Bool] b:Sym c:Sym];

print [Int Bool]->Void :- 
/*
  [([\a 0] $eq) \b] $and ? 'No more' print;
  [\a 0] $eq ? 'no more' print ; 
  \a print.
*/

  [\a 0] $eq ? \b ->Sym (\v ? 'No more'; 'no more') print;
  \a print.


opt_s Int->Void :-
  [\v 1] $eq $not ? 's' print; .

opt_s [Int Bool]->Void :-
  \a opt_s.

print [Int Sym Sym]->Void :-
  \a print \b print \a opt_s \c print.

print [[Int Bool] Sym Sym]->Void :-
  \a print \b print \a opt_s \c print.

bottles Int->Void :-
  [ \v 0 ] $eq ? 
     [ \v true ] print 
     ' bottles of beer on the wall, ' print
     [ \v false ] print 
     ' bottles of beer.\n' print 
     'Go to the store and buy some more, 99 bottles of beer on the wall.\n' print
;
  [\v ' bottle' ' of beer on the wall, '] print
  [\v ' bottle' ' of beer,\n'] print
  'Take one down, pass it around, ' print
  [\v 1] $sub ->Void ( 
      [ [\v false] ' bottle' ' of beer on the wall.\n\n' ] print
      \v bottles
      ).

bottles Void->Void :-
  99 bottles.




/*

bottles Int Void:
CALL_LIGHT bottles$1 Int Void
IF_NOT_FAIL{ EXIT }
CALL_LIGHT bottles$2 Int Void
IF_NOT_FAIL{ EXIT }
POP_FRAME
FAIL


bottles$1 Int Void:
...
LTE
IF_FALSE{FAIL}
...
CALL print
IF_FAIL{FAIL}
...
POP_FRAMEHEAD
EXIT

0
.a
EQ
IF_FALSE{FALSE}
CALL_LIGHT $2

*/
