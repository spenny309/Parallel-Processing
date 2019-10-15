use strict;
use Getopt::Long;

my $file1;
my $file2;
my $output = "valdiff.output";
my $start_index = 0;
my $end_index = 1536;

my @f1_list = ();
my @f2_list = ();
my @diff_list = ();

&GetOptions("file1=s" => \$file1, "file2=s" => \$file2, "output=s" =>
	    \$output, "start-index=i" => \$start_index, "end-index=i" => \$end_index);
print "Files to compare:";
print "\t $file1";
print "\t $file2";
print "\nOutput file:    \t $output\n";

&DoStuff();


sub DoStuff{
    @f1_list = &ReadInFile($file1);

    @f2_list = &ReadInFile($file2);
    &CompareFiles();
    &PrintToFile();
}

sub ReadInFile{
    my $filename = @_[0];
    my @r = ();
    my @g = ();
    my @b = ();
    my @s1 = ();
    my @s2 = ();
    my @s3 = ();

    print "Reading in file $filename\n";

    open(input_file, "$filename");
    while(<input_file>){

	my $index;
	my $val_r;
	my $val_g;
	my $val_b;
	my $val_s1;
	my $val_s2;
	my $val_s3;

	for ($_) {
	    s/^\s+//;
	    s/\s+$//;
	}  

	s/-/ -/g;
	s/:/: /g;

	
        my @fields = split(/\s+/, $_);
	if($#fields < 0){
	    next;
	}
	#    print "String " . $_ . "\n";

	for(my $i = 0; $i <= $#fields; $i++){
#print "$i " . @fields[$i] . "\n";	    
	    
	    my $index = @fields[0];
	    
	    if(@fields[$i] eq "R:"){
		$val_r = @fields[$i+1];
	    }

	    if(@fields[$i] eq "G:"){
		$val_g = @fields[$i+1];
	    }
	    
	    if(@fields[$i] eq "B:"){
		$val_b = @fields[$i+1];
	    }
	    
	    if(@fields[$i] eq "S0:"){
		$val_s1 = @fields[$i+1];
	    }

	    if(@fields[$i] eq "S1:"){	    
		$val_s2 = @fields[$i+1];
	    }

	    if(@fields[$i] eq "S2:"){
		$val_s3 = @fields[$i+1];
	    }
	}
	while(($index -1)> ($#r)){
	    push(@r, 0);
	    push(@g, 0);
	    push(@b, 0);
	    push(@s1, 0);
	    push(@s2, 0);
	    push(@s3, 0);
	}

	push(@r, $val_r);
	push(@g, $val_g);
	push(@b, $val_b);
	push(@s1, $val_s1);
	push(@s2, $val_s2);
	push(@s3, $val_s3);
#	print $#r . " ". $#g . " " . $#b . " " . $#s1 . " " . $#s2 ." " . $#s3 . "\n";
#	print "INSERTING $val_r $val_g $val_b $val_s1 $val_s2 $val_s3 \n";
#	exit;
    }
    close(input_file);

    while(255 > ($#r)){
	push(@r, 0);
	push(@g, 0);
	push(@b, 0);
	push(@s1, 0);
	push(@s2, 0);
	push(@s3, 0);
    }

    return (@r, @g, @b, @s1, @s2, @s3);

}

sub CompareFiles{
    
    my $sum_errors_r = 0;
    my $sum_errors_g = 0;
    my $sum_errors_b = 0;

    my $total_f1_r = 0;
    my $total_f1_g = 0;
    my $total_f1_b = 0;
    my $total_f2_r = 0;
    my $total_f2_g = 0;
    my $total_f2_b = 0;

    for(my $i = 0; $i < $#f1_list; $i++){
	my $d = abs((@f1_list[$i]) - (@f2_list[$i]));
#	print $i . " " . @f1_list[$i] . " " . @f2_list[$i] . " $d\n";

	push(@diff_list, $d);
	if($i < 256){
	    $sum_errors_r += $d;
	    $total_f1_r += @f1_list[$i];
	    $total_f2_r += @f2_list[$i];
	}
	elsif($i < 512){
	    $sum_errors_g += $d;
	    $total_f1_g += @f1_list[$i];
	    $total_f2_g += @f2_list[$i];
	}
	elsif($i < 768){
	    $sum_errors_b += $d;
	    $total_f1_b += @f1_list[$i];
	    $total_f2_b += @f2_list[$i];
	}
    }
    print "total file1: r $total_f1_r g $total_f1_g b $total_f1_b\n";
    print "total file2: r $total_f2_r g $total_f2_g b $total_f2_b\n";


    print "\nStatistics:\n";
    print "Sum of errors per color: \n";
    print "\t R $sum_errors_r G $sum_errors_g B $sum_errors_b\n";
    print "Total error: " . ($sum_errors_r + $sum_errors_g +
	$sum_errors_b) . "\n";

}

sub PrintToFile{

    open(OUTPUT, ">$output");
    my $i=0;
    print OUTPUT "R:\n";
    for($i; $i < 256; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }
    print OUTPUT "\nG:\n";
    for($i; $i < 512; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }
    print OUTPUT "\nB:\n";
    for($i; $i < 768; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }
    print OUTPUT "\nS1:\n";
    for($i; $i < 1024; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }
    print OUTPUT "\nS2:\n";
    for($i; $i < 1280; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }
    print OUTPUT "\nS3:\n";
    for($i; $i < 1536; $i++){
	print OUTPUT @diff_list[$i] . "\t";
    }

    close OUTPUT;
}

sub CreateInfo {
    my ($start, $end) = @_;  
    my @count = ();

    for(my $i = 0; $i < 768; $i++){
	push(@count, 0);
    }

    for(my $i = $start; $i < ($end); $i++){
	my $val = @diff_list[$i];
	(@count[$val])++;
    }
    return @count;
}

