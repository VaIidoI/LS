var number = 10;
call: main; return;

func printNumber:
	printl: "The number is: ";
	printl: number;
	number += 10;
end;

func main:
	call: printNumber;
	call: printNumber;
	call: printNumber;
	call: printNumber;
	call: printNumber;
end;


