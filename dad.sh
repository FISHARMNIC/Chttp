rm dad/main.out
clang dad/main.c lib/http.c -o dad/main.out
if [ $? ]; then
cd dad 
./main.out
echo "---exit code: $?---"
cd ../
./dispose.sh
fi