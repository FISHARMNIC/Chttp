gcc example/example.c -o example/main
if [ $? ]; then
cd example 
./main
echo "---exit code: $?---"
cd ../
./dispose.sh
fi