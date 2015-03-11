#!/bin/bash

cd unit-tests
echo "Running tests... any differences to correct values will be shown"
for test in *.pi; do
	echo Running "$test"
	cat "$test" | ../picalc -p &> temp
	diff temp "$test".result
done
rm temp
cd ..