/*
 * A simple utility to list top 10 committers with specified email postfix on repo
 *
 * There are many google committers in AOSP repo, then how to list top n committers from Nokia?
 * The idea is very simple - run 'git shortlog' on each repo projects and get output from piple. 
 * Then count it for the guys have '@nokia.com' postfix.
 *
 * History
 * -----------------------------------------------------------
 * 04/24/2014  Yang Ming<ming.3.yang@gmail.com>  Init version
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char * mail_postfix_filter[] = { "@nokia.com",
                                 "@microsoft.com",
                                 0 };

#define TOP_N 20 /* list top 10 by default */

/*
 * This git command can sort output according to the number of commits per author and provide 
 * a commit count summary. The output format like:
 *
 *   Numbers | Name   |EMail
 *   -------------------------------------------
 *    1888    m7yang   <ming.3.yang@nokia.com>
 *
 */
#define GIT_COMMAND  "git log --no-merges --after=\"2014-02-10\" --pretty=short|git shortlog -nse"
#define REPO_COMMAND "repo forall -c '"GIT_COMMAND"'"
#define MAX_AUTHOR_INFO_LEN 256

typedef struct COMMITTER_INFO {
	int    commits;     /* the number of commits for the author */
	char * author;      /* author name and email                */

	struct COMMITTER_INFO * next;
} COMMITTER_INFO, * P_COMMITTER_INFO;


COMMITTER_INFO * committers[26];

int is_in_filter_list(char * single_git_log)
{
	int n = 0;
	int ret = 0;
	while (mail_postfix_filter[n] != 0) {
        if(strstr(single_git_log,mail_postfix_filter[n]) != NULL)
            ret = 1;

		n++;
	}

	return ret;
}

void committer_free(void)
{
	int n;

	COMMITTER_INFO * ci;
	COMMITTER_INFO * next;

	for (n = 0; n < 26; n++) {

		ci = committers[n];
		while ( ci != NULL ) {
			next = ci->next;

			//printf(">>%s\n",ci->author);
			free(ci->author);
			free(ci);
			ci = next;
		}
	}
}

/* chech if p1 and p2 are same guy 
 * p1/p2 format is Name<Email>,e.g. Yang Ming<ming.3.yang@nokia.com>
 * p1 can be Yang Ming<ming.3.yang@nokia.com>, which can also be m7yang<ming.3.yang@micrsofot.com.That's why we need
 * this function to check...
 *
 * The idea is to check EMail part without postfix
 * */
int is_same_guy(char * p1, char * p2)
{
	int ret = 1;
	char * pcursor1;
	char * pcursor2;

	if (strcasecmp(p1, p2) == 0) {
		ret = 1;
	} else {
		pcursor1 = strstr(p1,"<");
		pcursor2 = strstr(p2,"<");

		while ( (pcursor1 != NULL && *pcursor1 != '@') &&
				(pcursor2 != NULL && *pcursor2 != '@'))   {

			if (*pcursor1 != *pcursor2) {
				ret = 0;
				break;
			}

			pcursor1++;
			pcursor2++;
		}

		//if (ret) printf("Same guy with diff mail:%s,%s\n",p1,p2);

	}

	return ret;
}

/*
 * insert author_info and commit_nums into committers[idx],which hold all committers's infor with 
 * same first character of his/her email 
 */
void linker_insert(int idx, char * author_info,int commit_nums)
{
	COMMITTER_INFO * pcursor = committers[idx];
	COMMITTER_INFO * pinsert = pcursor;
	COMMITTER_INFO * newnode;

	char * ai = author_info;

	if (pcursor == NULL) {
	
		/* the first item in the linker */
		pcursor = malloc(sizeof(COMMITTER_INFO));
		pcursor->commits = commit_nums;
		pcursor->author  = ai;
		pcursor->next    = NULL;
		committers[idx]  = pcursor;

	} else {

        /* merge or append */
		while (pcursor != NULL) {
			if (is_same_guy(pcursor->author, ai) == 0) {
				/* merge */
				pcursor->commits += commit_nums;
				free(ai);
				ai = NULL;
				break;
			}

			pinsert = pcursor;
			pcursor = pcursor->next;
		} /* end-while */

		if (ai != NULL) {
			/* append */
			newnode = malloc(sizeof(COMMITTER_INFO));
			newnode->commits = commit_nums;
			newnode->author  = ai;
			newnode->next    = NULL;
			pinsert->next    = newnode;
		}
	}
}

void committer_add(char * author_info, int commit_nums)
{
	char * p;

	/* insert single git info(author infor and commit numbers) to difference link based on author's first character */
	p = strstr(author_info,"<");
	p++;

	//printf("idx=%d,author=%s,commits=%d\n",tolower(*p)-'a',author_info,commit_nums);
	linker_insert((tolower(*p)-'a'), author_info, commit_nums); 
	
	return;
}


COMMITTER_INFO * pop_top_one(void)
{
	int n, max_commits = 0;
	COMMITTER_INFO * top_one = NULL;
	COMMITTER_INFO * ci;
	COMMITTER_INFO * ret = malloc(sizeof(COMMITTER_INFO));

	ret->author  = malloc(MAX_AUTHOR_INFO_LEN);
	ret->commits = 0;
	ret->next    = 0;

	for (n = 0; n < 26; n++) {
		
		if ((ci = committers[n]) == NULL) continue;
		while (ci != NULL) {
			if (ci->commits > max_commits) {
				max_commits = ci->commits;
				top_one = ci;
			}
			ci = ci->next;
		}		
	}

	if (max_commits == 0) {
		free(ret->author);
		free(ret);
		return NULL;
	}

	/* copy and then mark the top one to avoid to find this guy again for next call */
	ret->commits = top_one->commits;
	strncpy(ret->author,top_one->author,MAX_AUTHOR_INFO_LEN);
	top_one->commits = 0;	

	return ret;
}

int main( int argc, char *argv[] )
{
	FILE *fp;
	char single_git_log[1025]; /* commits name <email> */
	char * author_info;        /* name <email>         */
	int commit_nums;
	COMMITTER_INFO * top_one;
	int n;

	/* Open the command for reading. */
	fp = popen(REPO_COMMAND, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		exit(1);
	}

	/* Read the output a line at a time - sort it */
	while (fgets(single_git_log, sizeof(single_git_log)-1, fp) != NULL) {
		if (!is_in_filter_list(single_git_log)) 
			continue;

		author_info = malloc(MAX_AUTHOR_INFO_LEN);
		if (sscanf(single_git_log,"%d %[^\t\n]",&commit_nums,author_info) == 0) {
			free(author_info);
			continue;
		}

		committer_add(author_info,commit_nums);
	}

	for (n = 0; n < TOP_N; n++) {
		top_one = NULL;
		top_one = pop_top_one();
		if (top_one == NULL) break;
		printf("Top%d:%s:%d\n",n+1,top_one->author,top_one->commits);
		free(top_one);
	}

	committer_free();

	/* close */
	pclose(fp);

	return 0;
}
