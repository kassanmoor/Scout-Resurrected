Scout-Resurrected
==================

Scout was originally developed by Jeffrey Fulmer (joedog.org)  but was discontinued and replaced by Sproxy, a quite different tool, this repo start with v 0.86 and plans include fix to build it in OSX


Original Scout README 
by Jeffrey Fulmer
January 2002

                               
WHAT IS IT?
Scout is a utility which is distributed with Siege. Its role is 
to populate a text files with URLs for regression testing. When 
siege is invoked without a urls option [-u,  --url] it uses the
URLs in the urls.txt file and runs through them sequentially or 
randomly depending on the user's preference.  Without scout the 
process of filling that text file is cumbersome, copy and paste 
them from a web browser,  ask the intern to fill it,  pray to a 
diety, etc. Scout runs through a server, follows links & writes 
them to the file. 
                                     
WHY DO I NEED IT?
You don't NEED it,  but scout  makes life easier.  If you don't 
perform regression testing with siege then you don't need scout 
If you use siege to stress a single URL as follows:

siege -u http://bob.mackenzie.ca/sctv.jsp

then you will find  more relevant information here.  But if you 
want to perform regression testing,  then you will have to fill 
the urls.txt file. Scout was designed to make it easier. 
                                     
WHERE IS IT?
ftp://sid.joedog.org/pub/scout/scout-latest.tar.gz
                                     
INSTALLATION
Scout was built with GNU autoconf. If you are familiar with GNU 
software,  then  you  should  be  comfortable installing scout. 
Please consult the Scout INSTALL for more details.
                                     
DOCUMENTATION
Documentation is available in man pages: 
scout(1) see also siege(1)
                                     
LICENSE
Please consult COPYING for complete license information. 

Copyright (C) 2000-2002 Jeffrey Fulmer 

Permission is granted to anyone to make or  distribute verbatim 
copies of this  document as received,  in any medium,  provided 
that  the  copyright  notice  and  this  permission  notice are 
preserved, thus giving the recipient permission to redistribute 
in turn.  Permission is granted to distribute modified versions 
of this  document,  or of portions of it,  under the above con-
ditions,  provided  also  that  they  carry  prominent  notices 
stating who last changed them. 
  

