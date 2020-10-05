fileName=solver
g++ -D _REENTRANT $fileName.cpp -o $fileName  -lpthread
./$fileName  > $fileName .txt
rm $fileName