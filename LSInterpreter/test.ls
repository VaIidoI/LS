for i = 0, i < 1000, i++:
	printl: i;
	
	if i >= 15:
		printl: "breaking";
		continue;
	end;
end;