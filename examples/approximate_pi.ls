var max = 1000000; var primeCount = 0; var root = 0; var isPrime = false; var modulo = 0;
call: main; exit: 0;

func Root(num):
	sqrt: num;
	return: num;
end;

func Modulo(num, div):
	num %= div;
	delete: div; return: num;
end;

func main(): 
	for n = 2, n < max, n++:
		Root(n) >> root; isPrime = true;
		for i = 2, i <= root, i++:
			Modulo(n, i) >> modulo;
			
			if modulo == 0:
				isPrime = false; break;
			end;
		end;
		
		if isPrime == true:
			primeCount++;
		end;
	end;
	print: "In the first "; print: max; print: " numbers, there are "; print: primeCount; print: " prime numbers";
	var x;
	input: x; 
end;

