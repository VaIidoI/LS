# Always make sure to call the main method, return afterwards. 
call: main; exit: 0;

func printNumber(number, times):
	for i = 0, i < times, i++:
		printl: number;
	end;
end;

func main():
	printNumber(10, 5);
	printNumber(3, 2);
	printNumber(8, 10);
end;
