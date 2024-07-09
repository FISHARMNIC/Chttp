rm example/main.out
clang -O0 example/example.c -o example/main.out -g
if [ $? ]; then
cd example 
./main.out
echo "---exit code: $?---"
cd ../
./dispose.sh
fi