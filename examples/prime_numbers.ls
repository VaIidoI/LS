var max = 100000; var primeCount = 0; var root = 0; var isPrime = false; var modulo = 0;
call: main; return;

func IsPrime(number):
	root = number; sqrt: root;
	for i = 2, i <= root, i++:
		modulo = n; modulo %= i;
		
		if modulo == 0:
			# Push false to the stack and return.
			push: false; delete: number; delete: i; return;
		end;
	end;
	# Push false to the stack.
	push: true;
end;

func main(): 
	for n = 2, n < max, n++:
		IsPrime(n); pop: isPrime; 
		
		if isPrime == true:
			primeCount++;
		end;
	end;
	print: "In the first "; print: max; print: " numbers, there are "; print: primeCount; print: " prime numbers";
end;

