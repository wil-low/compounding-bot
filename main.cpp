// Copyright © 2017-2020 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWAnySigner.h>
#include <TrustWalletCore/TWCoinTypeConfiguration.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWString.h>

#include <iostream>
#include <string>

int main() {
    try {
        {
            std::cout << std::endl;
            std::cout << "  Wallet Core Demo, C++" << std::endl;
            std::cout << std::endl;
            std::cout << "  *** DISCLAIMER ***" << std::endl;
            std::cout << "  THIS IS A SAMPLE APPLICATION WITH DEMONSTRATION PURPOSES ONLY." << std::endl;
            std::cout << "  DO NOT USE WITH REAL SECRETS, REAL ADDRESSESS, OR REAL TRANSACTIONS.  USE IT AT YOUR OWN RISK." << std::endl;
            std::cout << "  *** DISCLAIMER ***" << std::endl;
            std::cout << std::endl;
        }

        TWHDWallet* walletImp = nullptr;
        {
            // Create a new multi-coin HD wallet, with new recovery phrase (mnemonic)
            std::cout << "Creating a new HD wallet ... ";
            TWHDWallet* walletNew = TWHDWalletCreate(128, TWStringCreateWithUTF8Bytes(""));
            std::cout << "done." << std::endl;
            std::cout << "Secret mnemonic for new wallet: '";
            std::cout << TWStringUTF8Bytes(TWHDWalletMnemonic(walletNew)) << "'." << std::endl;
            TWHDWalletDelete(walletNew);

            // Alternative: Import wallet with existing recovery phrase (mnemonic)
            std::cout << "Importing an HD wallet from earlier ... ";
            auto secretMnemonic = TWStringCreateWithUTF8Bytes("ripple scissors kick mammal hire column oak again sun offer wealth tomorrow wagon turn fatal");
            walletImp = TWHDWalletCreateWithMnemonic(secretMnemonic, TWStringCreateWithUTF8Bytes(""));
            TWStringDelete(secretMnemonic);
            std::cout << "done." << std::endl;
            std::cout << "Secret mnemonic for imported wallet: '";
            std::cout << TWStringUTF8Bytes(TWHDWalletMnemonic(walletImp)) << "'." << std::endl;
            std::cout << std::endl;
        }

        {
            // coin type: we use Ethereum
            const TWCoinType coinType = TWCoinType::TWCoinTypeEthereum; // TWCoinTypeBitcoin, TWCoinTypeEthereum
            std::cout << "Working with coin: " <<
                TWStringUTF8Bytes(TWCoinTypeConfigurationGetName(coinType)) << " " <<
                TWStringUTF8Bytes(TWCoinTypeConfigurationGetSymbol(coinType)) << std::endl;

            // Derive default address.
            std::cout << "Obtaining default address ... ";
            std::string address = TWStringUTF8Bytes(TWHDWalletGetAddressForCoin(walletImp, coinType));
            std::cout << " done." << std::endl;
            std::cout << "Default address:          '" << address << "'" << std::endl;

            // Alternative: Derive address using default derivation path.
            // Done in 2 steps: derive private key, then address from private key.
            // Note that private key is passed around between the two calls by the wallet -- be always cautious when handling secrets, avoid the risk of leaking secrets.
            std::cout << "Default derivation path:  " << TWStringUTF8Bytes(TWCoinTypeDerivationPath(coinType)) << std::endl;
            TWPrivateKey* secretPrivateKeyDefault = TWHDWalletGetKeyForCoin(walletImp, coinType);
            std::string addressDefault = TWStringUTF8Bytes(TWCoinTypeDeriveAddress(coinType, secretPrivateKeyDefault));
            std::cout << "Address from default key: '" << addressDefault << "'" << std::endl;

            // Alternative: Derive address using custom derivation path.  Done in 2 steps: derive private key, then address.
            auto customDerivationPath = TWStringCreateWithUTF8Bytes("m/44'/60'/1'/0/0");
            TWPrivateKey* secretPrivateKeyCustom = TWHDWalletGetKey(walletImp, coinType, customDerivationPath);
            TWStringDelete(customDerivationPath);
            std::string addressCustom = TWStringUTF8Bytes(TWCoinTypeDeriveAddress(coinType, secretPrivateKeyCustom));
            std::cout << "Custom-derived address:   '" << addressCustom << "'" << std::endl;
            std::cout << std::endl;

            std::cout << "RECEIVE funds: Perform send from somewehere else to this address:   " << address << " ." << std::endl;
            std::cout << std::endl;

            // Steps for sending:
            // 1. put together a send message (contains sender and receiver address, amount, gas price, etc.)
            // 2. sign this message
            // 3. broadcast this message to the P2P network -- not done in this sample
            // Note that Signer input and output are represented as protobuf binary messages, for which support is missing in C++.
            // Therefore some direct serialization/parsing is done in helper methods.
            std::cout << "SEND funds:" << std::endl;
            const std::string dummyReceiverAddress = "0xC37054b3b48C3317082E7ba872d7753D13da4986";
            auto secretPrivKey = TWPrivateKeyData(secretPrivateKeyDefault);

            std::cout << "preparing transaction (using AnySigner) ... ";
            std::string chainIdB64 = "AQ==";        // base64(parse_hex("01"))
            std::string gasPriceB64 = "1pOkAA==";   // base64(parse_hex("d693a4")) decimal 3600000000
            std::string gasLimitB64 = "Ugg=";       // base64(parse_hex("5208")) decimal 21000
            std::string amountB64 = "A0i8paFgAA=="; // base64(parse_hex("0348bca5a160"))  924400000000000
            std::string transaction = "{"
                "\"chainId\":\"" + chainIdB64 +
                "\",\"gasPrice\":\"" + gasPriceB64 +
                "\",\"gasLimit\":\"" + gasLimitB64 +
                "\",\"toAddress\":\"" + dummyReceiverAddress +
                "\",\"transaction\":{\"transfer\":{\"amount\":\"" + amountB64 +
                "\"}}}";            
            std::cout << "transaction: " << transaction << std::endl;

            std::cout << "signing transaction ... ";

            auto json = TWStringCreateWithUTF8Bytes(transaction.c_str());
            auto result = TWAnySignerSignJSON(json, secretPrivKey, TWCoinTypeEthereum);
            auto signedTransaction = std::string(TWStringUTF8Bytes(result));
            std::cout << "done" << std::endl;
            std::cout << "Signed transaction data (to be broadcast to network):  (len " << signedTransaction.length() << ") '" << signedTransaction << "'" << std::endl;
            // see e.g. https://github.com/flightwallet/decode-eth-tx for checking binary output content
            std::cout << std::endl;
            TWStringDelete(json);
            TWStringDelete(result);
        }
        std::cout << "Bye!" << std::endl;
        TWHDWalletDelete(walletImp);
    } catch (const std::exception& ex) {
        std::cout << "exception: " << ex.what() << std::endl;
        throw ex;
    }
}
