# Entry point
call: main; exit: 0;

func Test(number):
	if number == 4:
		delete: number; return;
	end;
	
	if number != 4:
		delete: number; return: "lucky";
	end;
end;
	

func main():
	for i = 0, i <= 10, i++:
		Test(i) >> luck;
		print: i; print: " is: ";
		
		if luck: printl:
			luck; 
		end;
		if !luck:
			printl: "unlucky";
		end;
		delete: luck;
	end;
end;
