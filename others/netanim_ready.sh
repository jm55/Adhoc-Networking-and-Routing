#Source: https://www.youtube.com/watch?v=HRAjKRrvgh4

#Assumes the use of ns-allinone-3.36.1/netanim-3.108/
ls ~/ns-allinone-3.36.1/netanim-3.108/

#Making/Building NetAnim
make clean
qmake NetAnim.pro
make #This command may take a while to execute, just like how NS3 was built using ./build.py

#Launching NetAnim (from netanim directory)
./NetAnim