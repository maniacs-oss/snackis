#!/usr/local/bin/snabel

/*
   postag.sl

   Usage:
   cat *.txt | postag.sl
*/

func: guess-tag {
  let: w; _
  
  [{@w 'ed' suffix?}
     #VBD.
   {@w 'es' suffix?}
     #VBZ.
   {@w 'ing' suffix?}
     #VBG.
   {@w 'ould' suffix?}
     #MD.
   {@w '\'s' suffix?}
     #NN$.
   {@w 's' suffix?}
     #NNS.
   {true}
     #NN.] cond
};

let: lookup [
  'a'    #AT.
  'and'  #CC.
  'are'  #BER.
  'in'   #IN.
  'is'   #BEZ.
  'of'   #IN.
  'on'   #IN.
  'our'  #PP$.
  'so'   #QL.
  'that' #CS.
  'the'  #AT.
  'this' #DT.
  'was'  #BEDZ.
  'what' #WDT.
] table;

func: get-tag {
  $ @lookup $1 get &nop &guess-tag if
};

stdin read unopt
words unopt
{$ downcase} map
{$ get-tag '$1\t$0' say _ _} for