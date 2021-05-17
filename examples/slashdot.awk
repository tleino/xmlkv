BEGIN { 
	while (getline < "in") {
		guid[$0]=1
	}
RS="";
FS="\n"; }
$1 ~ /link/ {
	gsub(/\t/,"");
	print $2 | "sha1 -b | cut -b-4";
	close("sha1 -b | cut -b-4");
}
$1 ~ /title/ { 
	gsub(/\t/,"");
	for(a=2; a<=NF; a++) 
		print toupper($a) | "fmt 65";
	close("fmt 65")
}
$1 ~ /description/ { 
	gsub(/\t/,"");
	cmd="lynx -dump -stdin -nolist -hiddenlinks=ignore|sed '/^$/q'|fmt 55"
	for(a=2; a<=NF; a++) 
		print $a | cmd
	close(cmd)
}
