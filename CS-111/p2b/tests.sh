
#lab2b_1.png
./lab2_list --threads=1  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=2  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=4  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=8  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=12  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=16  --iterations=1000 --sync=m >> lab2b_list.csv
./lab2_list --threads=24  --iterations=1000 --sync=m >> lab2b_list.csv
#lab2b_2.png
./lab2_list --threads=1  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=2  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=4  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=8  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=12  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=16  --iterations=1000 --sync=s >> lab2b_list.csv
./lab2_list --threads=24  --iterations=1000 --sync=s >> lab2b_list.csv

#lab2b_3.png
for i in 1 4 8 12 16
do
    for j in 1 2 4 8 16
    do
	./lab2_list --iterations=$j --threads=$i --yield=id --lists=4 >>lab2b_list.csv
	done

    for j in 10 20 40 80
    do
	./lab2_list --iterations=$j --threads=$i --yield=id --lists=4 --sync=m >>lab2b_list.csv

	./lab2_list --iterations=$j --threads=$i --yield=id --lists=4 --sync=s >>lab2b_list.csv
	done
done

#lab2_4.png
for i in 1 2 4 8 12
do
    for j in 4 8 16
  	do
	./lab2_list --iterations=1000 --lists=$j --threads=$i --sync=m 1>>lab2b_list.csv
	
	done
done

#lab2_5.png
for i in 1 2 4 8 12
do
    for j in 4 8 16
  	do
	./lab2_list --iterations=1000 --lists=$j --threads=$i --sync=s 1>>lab2b_list.csv

	done
done





