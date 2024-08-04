#!/bin/sh

cd $(dirname $0)

find . -type f -a \( \
		-name \*.md -o \( \
			-name \*.html -a \
			\! -path ./tutorial/index.html -a \
			\! -path ./tutorial/0\*.html \
		\) \
		-o -name toc \
		-o -name top.html \
		-o -name footer.html \
	\) \
| entr -rs "echo 'Regenerating ...' && ./serve.sh"
