#########################################################################
# File Name:chgemail.sh
# Author: sudoku.huang
# mail: sudoku.huang@gmail.com
# Created Time:Wed Feb 20 10:00:00 2019
#########################################################################
#!/bin/bash

#!/bin/sh

git filter-branch --env-filter '

OLD_EMAIL="want-to-replace-email"
CORRECT_NAME="sudoku.huang"
CORRECT_EMAIL="sudoku.huang@gmail.com"

if [ "$GIT_COMMITTER_EMAIL" = "$OLD_EMAIL" ]
then
    export GIT_COMMITTER_NAME="$CORRECT_NAME"
    export GIT_COMMITTER_EMAIL="$CORRECT_EMAIL"
fi
if [ "$GIT_AUTHOR_EMAIL" = "$OLD_EMAIL" ]
then
    export GIT_AUTHOR_NAME="$CORRECT_NAME"
    export GIT_AUTHOR_EMAIL="$CORRECT_EMAIL"
fi
' --tag-name-filter cat -- --branches --tags
#git push origin --force --all
