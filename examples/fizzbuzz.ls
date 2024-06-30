call: main; exit 0;

func FizzBuzz(number):
	var bFizzOrBuzz = false;
	var modulo = number; modulo %= 3;
	if modulo == 0:
		bFizzOrBuzz = true;
		print: "Fizz";
	end;
	
	modulo = number; modulo %= 5;
	if modulo == 0:
		bFizzOrBuzz = true;
		print: "Buzz";
	end;
	
	if bFizzOrBuzz == false:
		print: number;
	end;
	
	# Scopes haven't been implemented yet ;(
	delete: bFizzOrBuzz; delete: modulo;
end;

func main():
	for i = 1, i <= 15, i++:
		FizzBuzz(i); endl;
	end;
end;
