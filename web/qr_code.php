<?

error_reporting(0);

function getCnVersion($v)
{
	if($v[4]=="J")
	{
		return "JPN";
	}else{
		return "WEST";
	}
}

function getFirmVersion($v)
{
	if($v[5]=="NEW")
	{
		return "N3DS";
	}else{
		if($v[0]<5)
		{
			return "PRE5";
		}else{
			return "POST5";
		}
	}
}

function getMenuVersion($v)
{
	if($v[0]==9 && $v[1]==7)
	{
		return "17415";
	}else{
		return "12288";
	}
}

$version = array(
		0 => $_POST['zero'],
		1 => $_POST['one'],
		2 => $_POST['two'],
		3 => $_POST['three'],
		4 => $_POST['four'],
		5 => $_POST['five']
	);

$filename="./unsupported.png";

// check that version is valid-ish
if(is_numeric($version[0]) && is_numeric($version[1]) && is_numeric($version[2]) && is_numeric($version[3]))
{
	$filename="./q/".getFirmVersion($version)."_".getCnVersion($version)."_".getMenuVersion($version).".png";
}

if(!file_exists($filename))
{
	$filename="./unsupported.png";
}

$fp = fopen($filename, 'rb');

// // send the right headers
header("Content-Type: image/png");
header("Content-Length: " . filesize($filename));

// dump the picture and stop the script
fpassthru($fp);

exit;

?>
