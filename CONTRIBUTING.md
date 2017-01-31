# How to Contribute

Thank you for your interest in contributing to SCRAM.
Contributions are accepted through [GitHub](https://help.github.com)
Pull Requests and Issue Tracker.
Best practices are encouraged:

* [Git SCM](http://git-scm.com/)
* [Branching Model](http://nvie.com/posts/a-successful-git-branching-model/)
* [Writing Good Commit Messages](https://github.com/erlang/otp/wiki/Writing-good-commit-messages)
* [On Commit Messages](http://who-t.blogspot.com/2009/12/on-commit-messages.html)
* [Atomic Commit](https://en.wikipedia.org/wiki/Atomic_commit#Atomic_commit_convention)


## Developer Workflow

1. Start by forking this repository and setting it as the upstream repository.
2. Create your **topic** branch from the **develop** branch.
3. Keep in sync your **origin** develop branch with the **upstream** develop branch.
4. Develop your contributions following the [Coding Style and Quality Assurance].
   Every commit should compile and pass tests.
5. Keep your **topic** branch in sync with the **develop** branch
   by merging or rebasing your **topic** branch on top of the **develop**.
   Rebasing is highly recommended for streamlining the history.
   However, **DO NOT** rebase any commits
   that have been pulled/pushed anywhere else other than your own fork.
6. Use the [developer mailing list] and Issue Tracker
   to stay in touch with the project development.
7. Submit your [pull request] from **your topic** branch to the **upstream develop** branch.
8. Your pull request will be reviewed by another developer before merging.

[Coding Style and Quality Assurance]: https://scram-pra.org/doc/coding_standards.html
[developer mailing list]: https://groups.google.com/forum/#!forum/scram-dev
[pull request]: https://help.github.com/articles/using-pull-requests/


## Contributor License Agreement

Upon pull requests,
first time contributors will be asked to sign the [Contributor License Agreement]
through [CLA Assistant] with a GitHub account.
This license is for your protection as a contributor
as well as the protection of SCRAM and its users;
it does not change your rights to use your own contributions for any other purpose.

[Contributor License Agreement]: https://github.com/rakhimov/scram/blob/develop/ICLA.md
[CLA Assistant]: https://cla-assistant.io/


## Contributor Code of Conduct

Please note that this project is released with a [Contributor Code of Conduct].
By participating in this project,
you agree to abide by its terms.

[Contributor Code of Conduct]: https://github.com/rakhimov/scram/blob/develop/CODE_OF_CONDUCT.md
