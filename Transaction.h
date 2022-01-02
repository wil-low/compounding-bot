#pragma once

#include <uint256.h>
#include <string>

struct Transaction 
{
	TW::uint256_t timestamp_;
	int index_;
	std::string from_;
	std::string to_;
	int log_count_;
	double tx_fee_;
	std::string hash_;
	TW::uint256_t block_number_;
	TW::uint256_t gas_limit_;
	TW::uint256_t gas_price_;
	TW::uint256_t gas_used_;
	int status_;
	int bot_id_;
	int delta_msec_;
};
 