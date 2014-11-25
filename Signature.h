#ifndef SIGNATURE_H__
#define SIGNATURE_H__

#include <inttypes.h>
#include <cryptopp/dsa.h>
#include <cryptopp/asn.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/eccrypto.h>
#include "CryptoConst.h"

namespace i2p
{
namespace crypto
{
	class Verifier
	{
		public:
			
			virtual ~Verifier () {};
			virtual bool Verify (const uint8_t * buf, size_t len, const uint8_t * signature) const = 0;
			virtual size_t GetPublicKeyLen () const = 0;
			virtual size_t GetSignatureLen () const = 0;
	};

	class Signer
	{
		public:

			virtual ~Signer () {};		
			virtual void Sign (CryptoPP::RandomNumberGenerator& rnd, const uint8_t * buf, int len, uint8_t * signature) const = 0; 
	};

	const size_t DSA_PUBLIC_KEY_LENGTH = 128;
	const size_t DSA_SIGNATURE_LENGTH = 40;	
	const size_t DSA_PRIVATE_KEY_LENGTH = DSA_SIGNATURE_LENGTH/2;
	class DSAVerifier: public Verifier
	{
		public:

			DSAVerifier (const uint8_t * signingKey)
			{
				m_PublicKey.Initialize (dsap, dsaq, dsag, CryptoPP::Integer (signingKey, DSA_PUBLIC_KEY_LENGTH));
			}
	
			bool Verify (const uint8_t * buf, size_t len, const uint8_t * signature) const
			{
				CryptoPP::DSA::Verifier verifier (m_PublicKey);
				return verifier.VerifyMessage (buf, len, signature, DSA_SIGNATURE_LENGTH);
			}	

			size_t GetPublicKeyLen () const { return DSA_PUBLIC_KEY_LENGTH; };
			size_t GetSignatureLen () const { return DSA_SIGNATURE_LENGTH; };
			
		private:

			CryptoPP::DSA::PublicKey m_PublicKey;
	};

	class DSASigner: public Signer
	{
		public:

			DSASigner (const uint8_t * signingPrivateKey)
			{
				m_PrivateKey.Initialize (dsap, dsaq, dsag, CryptoPP::Integer (signingPrivateKey, DSA_PRIVATE_KEY_LENGTH));
			}

			void Sign (CryptoPP::RandomNumberGenerator& rnd, const uint8_t * buf, int len, uint8_t * signature) const
			{
				CryptoPP::DSA::Signer signer (m_PrivateKey);
				signer.SignMessage (rnd, buf, len, signature);
			}

		private:

			CryptoPP::DSA::PrivateKey m_PrivateKey;
	};

	inline void CreateDSARandomKeys (CryptoPP::RandomNumberGenerator& rnd, uint8_t * signingPrivateKey, uint8_t * signingPublicKey)
	{
		CryptoPP::DSA::PrivateKey privateKey;
		CryptoPP::DSA::PublicKey publicKey;
		privateKey.Initialize (rnd, dsap, dsaq, dsag);
		privateKey.MakePublicKey (publicKey);
		privateKey.GetPrivateExponent ().Encode (signingPrivateKey, DSA_PRIVATE_KEY_LENGTH);	
		publicKey.GetPublicElement ().Encode (signingPublicKey, DSA_PUBLIC_KEY_LENGTH);
	}	

	template<typename Hash, size_t keyLen>
	class ECDSAVerifier: public Verifier
	{		
		public:

			template<typename Curve>
			ECDSAVerifier (Curve curve, const uint8_t * signingKey)
			{
				m_PublicKey.Initialize (curve, 
					CryptoPP::ECP::Point (CryptoPP::Integer (signingKey, keyLen/2), 
					CryptoPP::Integer (signingKey + keyLen/2, keyLen/2)));
			}

			bool Verify (const uint8_t * buf, size_t len, const uint8_t * signature) const
			{
				typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::Verifier verifier (m_PublicKey);
				return verifier.VerifyMessage (buf, len, signature, keyLen); // signature length
			}	

			size_t GetPublicKeyLen () const { return keyLen; };
			size_t GetSignatureLen () const { return keyLen; }; // signature length = key length
			
		private:

			typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::PublicKey m_PublicKey;
	};

	template<typename Hash>
	class ECDSASigner: public Signer
	{
		public:

			template<typename Curve>
			ECDSASigner (Curve curve, const uint8_t * signingPrivateKey, size_t keyLen)
			{
				m_PrivateKey.Initialize (curve, CryptoPP::Integer (signingPrivateKey, keyLen/2)); // private key length
			}

			void Sign (CryptoPP::RandomNumberGenerator& rnd, const uint8_t * buf, int len, uint8_t * signature) const
			{
				typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::Signer signer (m_PrivateKey);
				signer.SignMessage (rnd, buf, len, signature);
			}

		private:

			typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::PrivateKey m_PrivateKey;
	};

	template<typename Hash, typename Curve>
	inline void CreateECDSARandomKeys (CryptoPP::RandomNumberGenerator& rnd, Curve curve, 
		size_t keyLen, uint8_t * signingPrivateKey, uint8_t * signingPublicKey)
	{
		typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::PrivateKey privateKey;
		typename CryptoPP::ECDSA<CryptoPP::ECP, Hash>::PublicKey publicKey;
		privateKey.Initialize (rnd, curve);
		privateKey.MakePublicKey (publicKey);
		privateKey.GetPrivateExponent ().Encode (signingPrivateKey, keyLen/2);	
		auto q = publicKey.GetPublicElement ();
		q.x.Encode (signingPublicKey, keyLen/2);
		q.y.Encode (signingPublicKey + keyLen/2, keyLen/2);
	}	

// ECDSA_SHA256_P256
	const size_t ECDSAP256_KEY_LENGTH = 64;	
	class ECDSAP256Verifier: public ECDSAVerifier<CryptoPP::SHA256, ECDSAP256_KEY_LENGTH>
	{
		public:

			ECDSAP256Verifier (const uint8_t * signingKey): 
				ECDSAVerifier (CryptoPP::ASN1::secp256r1(), signingKey)
			{
			}			
	};	

	class ECDSAP256Signer: public ECDSASigner<CryptoPP::SHA256>
	{
		public:

			ECDSAP256Signer (const uint8_t * signingPrivateKey):
				ECDSASigner (CryptoPP::ASN1::secp256r1(), signingPrivateKey, ECDSAP256_KEY_LENGTH)
			{
			}
	};

	inline void CreateECDSAP256RandomKeys (CryptoPP::RandomNumberGenerator& rnd, uint8_t * signingPrivateKey, uint8_t * signingPublicKey)
	{
		CreateECDSARandomKeys<CryptoPP::SHA256> (rnd, CryptoPP::ASN1::secp256r1(), ECDSAP256_KEY_LENGTH, signingPrivateKey, signingPublicKey);
	}	

// ECDSA_SHA384_P384
	const size_t ECDSAP384_KEY_LENGTH = 96;
	class ECDSAP384Verifier: public ECDSAVerifier<CryptoPP::SHA384, ECDSAP384_KEY_LENGTH>
	{
		public:

			ECDSAP384Verifier (const uint8_t * signingKey): 
				ECDSAVerifier (CryptoPP::ASN1::secp384r1(), signingKey)
			{
			}			
	};	

	class ECDSAP384Signer: public ECDSASigner<CryptoPP::SHA384>
	{
		public:

			ECDSAP384Signer (const uint8_t * signingPrivateKey):
				ECDSASigner (CryptoPP::ASN1::secp384r1(), signingPrivateKey, ECDSAP384_KEY_LENGTH)
			{
			}
	};

	inline void CreateECDSAP384RandomKeys (CryptoPP::RandomNumberGenerator& rnd, uint8_t * signingPrivateKey, uint8_t * signingPublicKey)
	{
		CreateECDSARandomKeys<CryptoPP::SHA384> (rnd, CryptoPP::ASN1::secp384r1(), ECDSAP384_KEY_LENGTH, signingPrivateKey, signingPublicKey);
	}	

// ECDSA_SHA512_P521
	const size_t ECDSAP521_KEY_LENGTH = 132;
	class ECDSAP521Verifier: public ECDSAVerifier<CryptoPP::SHA512, ECDSAP521_KEY_LENGTH>
	{
		public:

			ECDSAP521Verifier (const uint8_t * signingKey): 
				ECDSAVerifier (CryptoPP::ASN1::secp521r1(), signingKey)
			{
			}			
	};	

	class ECDSAP521Signer: public ECDSASigner<CryptoPP::SHA512>
	{
		public:

			ECDSAP521Signer (const uint8_t * signingPrivateKey):
				ECDSASigner (CryptoPP::ASN1::secp521r1(), signingPrivateKey, ECDSAP521_KEY_LENGTH)
			{
			}
	};

	inline void CreateECDSAP521RandomKeys (CryptoPP::RandomNumberGenerator& rnd, uint8_t * signingPrivateKey, uint8_t * signingPublicKey)
	{
		CreateECDSARandomKeys<CryptoPP::SHA512> (rnd, CryptoPP::ASN1::secp521r1(), ECDSAP521_KEY_LENGTH, signingPrivateKey, signingPublicKey);
	}
	
}
}

#endif

