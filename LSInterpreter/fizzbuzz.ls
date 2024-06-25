# --- Define Global Variables ---
var i = 0; var max = 10; var modolo = 0; var bDid = false; #bDid checks if i was either fizz or buzz
# --- Define Global Variables ---
jump: for; printl: "Returned to beginning"; return;
	
=for; if: i >= max, end;
	#Increment i and reset bDid
	i++; bDid = false;
	
	#Check if i is divisible by 3, if yes jump to fizz
	modolo = i; modolo %= 3;
	if: modolo != 0, end_if_fizz;
		bDid = true;
		print: "Fizz";
	=end_if_fizz;
	
	#Check if i is divisible by 5, if yes jump to buzz
	modolo = i; modolo %= 5;
	if: modolo != 0, end_if_buzz;
		bDid = true;
		print: "Buzz";
	=end_if_buzz;
	
	#If i was neither divisible by 3 or 5, print it.
	if: bDid != false, end_if_nothing;
		print: i;
	=end_if_nothing;
	
	# Push a linebreak to console
	endl;
if: i < max, for; # As long as i is smaller than max, repeat the process.
=end;
