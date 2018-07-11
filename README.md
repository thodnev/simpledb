#### SimpleDB
A (very) simple database written in C [for training purposes]

![simpledb](https://user-images.githubusercontent.com/16870636/42594254-558c1baa-8557-11e8-8a05-5cd541a52c63.png)

The code is written to show an example of the implementation of "classic" console applications in Linux using the C language. The original code is working, but it is not without errors (which I'm accustomed of) and does not implement the declared functionality to the fullest.

#### Commiting to the repository:

- Make sure you're introducing a small subset of changes with your commit
- The resulting program should be working (no regressions allowed)
- It should build fine with `CFLAGS` already present in Makefile (`-O2 -Wall -Wextra -Wpedantic -Werror`)
- Please do not include unneeded files in your commit
- Try to keep your git history clean
- Put valuable information in description of your Pull request:
  - what's been implemented or fixed
  - brief description of how it works
  - how it differs from the already existing implementation (new functions introduced, logic changed, ...)
  - what are the benefits
  
  Please keep the PR description short and informative
- Ask someone [to review](https://help.github.com/articles/reviewing-proposed-changes-in-a-pull-request) your proposed changes
- Respect the [CodingStyle](https://github.com/thodnev/simpledb/files/2185946/CodingStyle1.pdf)
- Run a lint check against your code and attach the generated report to PR
- All PRs are merged after there are no reviews questions to the code being added (LFTM mark)