var max = 10000; var primeCount = 0;

var root = 0; var isPrime = false; var modulo = 0;
for n = 2, n < max, n++:
	root = n; sqrt: root; isPrime = true;
	for i = 2, i <= root, i++:
		modulo = n; modulo %= i;
		
		if modulo == 0:
			isPrime = false; break;
		end;
	end;
	
	if isPrime == true:
		primeCount++;
	end;
	
end;

print: "In the first "; print: max; print: " numbers, there are "; print: primeCount; print: " prime numbers";