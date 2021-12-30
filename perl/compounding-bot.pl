#!/usr/bin/perl

# Requires:
# cpan App::cpanminus
# cpanm Digest::Keccak

use strict;
use warnings;
no warnings 'portable';

use HTTP::Request;
use LWP::UserAgent;
use JSON;
use Data::Dumper;
use Math::BigInt;
use Digest::Keccak qw(keccak_256_hex);

# mainnet
=head
my $WALLET = '9ee8d57c11a1b70fd4373e75b6e1d54c5cf458df';
my $SECRET = '';
my $CONTRACT = 'f35848441017917a034589bfbec4b3783bb39cb2';
my $URL = 'https://bsc-dataseed.binance.org/';

my %ETH_CALLS = (
	userInfo => '1959a002',	# userInfo(address)
	balanceOf =>'70a08231',	# balanceOf(address)
	nearestCompoundingTime =>'a5aed60b',	# nearestCompoundingTime()
	canCompound =>'ec8079d3',	# canCompound()
	compound =>'f69e2047',	# compound()
	deposit =>'f69e2047',	# deposit(uint256,address)
);
=cut

#testnet
# BNB contract
# https://testnet.bscscan.com/address/0xcd5dc972dbc4df70f64871d87ae8f64d32988279#readContract

my $WALLET = '9777d3d77f4cc000ed6aa6979854178a28e04eb5';
my $SECRET = 'b7cf1604a1f1f57765ee9613ea5188ac077ab82155d87f749c86e39eaec53c15';
my $CONTRACT = 'cd5dc972dbc4df70f64871d87ae8f64d32988279';
my $URL = 'https://data-seed-prebsc-1-s1.binance.org:8545/';

my %ETH_CALLS = (
	userInfo => meth_sig('userInfo(address)'),
	balanceOf => meth_sig('balanceOf(address)'),
	nearestCompoundingTime => meth_sig('nearestCompoundingTime()'),
	canCompound => meth_sig('canCompound()'),
	compound => meth_sig('compound()'),
	deposit => meth_sig('deposit(uint256,address)'),
	getOwner => meth_sig('getOwner()'),
	symbol => meth_sig('symbol()'),
	approve => meth_sig('approve()'),
	allowance => meth_sig('allowance(address,address)'),
);

#die hex('0000000000000000000000000000000000000000000000000000000061c9a37e');

#my $content = eth_call('allowance', [$WALLET, $WALLET]);
#my $content = eth_call('approve', [$WALLET, '1']);
#my $content = eth_signed_transaction('approve', [$WALLET, '1']);
my $content = eth_sendTransaction('approve', [$WALLET, '1']);

print "Request content:\n$content\n";

my $ua = LWP::UserAgent->new;

my $req = HTTP::Request->new(POST=>$URL);
# указываем тип контекста
$req->content_type('application/json');
# вводим сам контекст для передачи
$req->content($content);

my $res = $ua->request($req);

# получаем ответ от выводим его
print 'Raw content: ' . $res->content . "\n";

my $json_response = decode_json($res->content);
print Dumper($json_response);

if (defined ($json_response->{error})) {
	die 'Error message: '. $json_response->{error}->{message} . "\n";
}
else {
	my $result = $json_response->{result};
	$result =~ s/^0x//;
	my $len = length($result);
	die "Wrong length: $len\n" if $len % 64;
	print "Response fields:\n";
	while ($result =~ s/^(\w{64})//) {
		#print Math::BigInt->new($1)->as_hex;
		print "\t" . $1 . ': ' . hex($1) . "\n";
	}
}
		

sub abi_pad {
	my $in = shift();
	my $len = length($in);
	return ('0' x (64 - $len)) . $in;
}

sub meth_sig { # method signature
	my $method = shift();
	return substr(keccak_256_hex($method), 0, 8);
}

sub eth_call { # method name, array_ref
	my ($method, $params) = @_;
	die "No method: $method" if !defined($ETH_CALLS{$method});

	my $data = '0x'. $ETH_CALLS{$method};
	foreach (@$params) {
		$data .= abi_pad($_);
	}
	my $json_data = {
		method => 'eth_call',
		id => 1,
		jsonrpc => '2.0',
		params => [
			{
				from => '0x' . $WALLET,
				to => '0x' . $CONTRACT,
				data => $data
			},
			'latest'
		]
	};

	return encode_json($json_data);
}

sub eth_gasPrice {
	my $json_data = {
		method => 'eth_gasPrice',
		id => 1,
		jsonrpc => '2.0',
		params => []
	};

	return encode_json($json_data);
}

sub eth_sendTransaction { # method name, array_ref
	my ($method, $params) = @_;
	die "No method: $method" if !defined($ETH_CALLS{$method});

	my $data = '0x'. $ETH_CALLS{$method};
	foreach (@$params) {
		$data .= abi_pad($_);
	}
	my $json_data = {
		method => 'eth_sendTransaction',
		id => 1,
		jsonrpc => '2.0',
		params => [
			{
				from => '0x' . $WALLET,
				to => '0x' . $CONTRACT,
				data => $data
			},
		]
	};

	return encode_json($json_data);
}
