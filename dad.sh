rm dad/main.out
clang -O0 dad/main.c -o dad/main.out
if [ $? ]; then
cd dad 
./main.out
echo "---exit code: $?---"
cd ../
./dispose.sh
fi