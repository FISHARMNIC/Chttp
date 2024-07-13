rm example/main.out
clang example/example.c lib/main.c -o example/main.out -g
if [ $? ]; then
cd example 
./main.out
echo "---exit code: $?---"
cd ../
./dispose.sh
fi