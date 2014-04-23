topN_repo_committers
====================

A simple utility to list top 10 committers with specified email postfix on repo

There are many google committers in AOSP repo, then how to list top n committers from Nokia?
The idea is very simple - run 'git shortlog' on each repo projects and get output from piple. 
Then count it. for example counting the committers with '@nokia.com' postfix.
