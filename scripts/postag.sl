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

stdin read unopt
words unopt
{$ downcase} map
{$ guess-tag '$1\t$0' say _ _} for