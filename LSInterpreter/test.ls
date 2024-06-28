var max = 10000; var primeCount = 0;

for number = 0, number < max, number++:
	var timesDivisible = 0; var root = number; sqrt: root;
	
	for i = 2, i < root, i++:
		var modulo = number; modulo %= i;

		if modulo == 0:
			timesDivisible++;
		end;
		
		delete: modulo;
	end;
	
	if timesDivisible == 0:
		primeCount++;
	end;
	
	delete: timesDivisible; delete: root;
end;

print: "In the first "; print: max; print: " numbers there are "; print: primeCount; printl: " primes";