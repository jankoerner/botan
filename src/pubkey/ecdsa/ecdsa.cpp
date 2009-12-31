/*
* ECDSA implemenation
* (C) 2007 Manuel Hartl, FlexSecure GmbH
*     2007 Falko Strenzke, FlexSecure GmbH
*     2008 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/ecdsa.h>
#include <botan/numthry.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/secmem.h>
#include <botan/point_gfp.h>

namespace Botan {

ECDSA_PrivateKey::ECDSA_PrivateKey(RandomNumberGenerator& rng,
                                   const EC_Domain_Params& dom_pars)
   {
   mp_dom_pars = std::auto_ptr<EC_Domain_Params>(new EC_Domain_Params(dom_pars));
   generate_private_key(rng);

   try
      {
      mp_public_point->check_invariants();
      }
   catch(Illegal_Point& e)
      {
      throw Invalid_State("ECDSA key generation failed");
      }

   m_ecdsa_core = ECDSA_Core(*mp_dom_pars, m_private_value, *mp_public_point);
   }

ECDSA_PrivateKey::ECDSA_PrivateKey(const EC_Domain_Params& domain,
                                   const BigInt& x)
   {
   mp_dom_pars = std::auto_ptr<EC_Domain_Params>(new EC_Domain_Params(domain));

   m_private_value = x;
   mp_public_point = std::auto_ptr<PointGFp>(new PointGFp (mp_dom_pars->get_base_point()));
   mp_public_point->mult_this_secure(m_private_value,
                                     mp_dom_pars->get_order(),
                                     mp_dom_pars->get_order()-1);

   try
      {
      mp_public_point->check_invariants();
      }
   catch(Illegal_Point& e)
      {
      throw Invalid_State("ECDSA key generation failed");
      }

   m_ecdsa_core = ECDSA_Core(*mp_dom_pars, m_private_value, *mp_public_point);
   }

/*
* ECDSA_PublicKey
*/
void ECDSA_PublicKey::affirm_init() const // virtual
   {
   EC_PublicKey::affirm_init();
   }

void ECDSA_PublicKey::set_domain_parameters(const EC_Domain_Params& dom_pars)
   {
   if(mp_dom_pars.get())
      {
      // they are already set, we must ensure that they are equal to the arg
      if(dom_pars != *mp_dom_pars.get())
         throw Invalid_Argument("EC_PublicKey::set_domain_parameters - cannot reset to a new value");

      return;
      }

   if(m_enc_public_point.size() == 0)
      throw Invalid_State("EC_PublicKey::set_domain_parameters(): encoded public point isn't set");

   // now try to decode the public key ...
   PointGFp tmp_pp(OS2ECP(m_enc_public_point, dom_pars.get_curve()));
   try
      {
      tmp_pp.check_invariants();
      }
   catch(Illegal_Point e)
      {
      throw Invalid_State("EC_PublicKey::set_domain_parameters(): point does not lie on provided curve");
      }

   std::auto_ptr<EC_Domain_Params> p_tmp_pars(new EC_Domain_Params(dom_pars));
   ECDSA_Core tmp_ecdsa_core(*p_tmp_pars, BigInt(0), tmp_pp);
   mp_public_point.reset(new PointGFp(tmp_pp));
   m_ecdsa_core = tmp_ecdsa_core;
   mp_dom_pars = p_tmp_pars;
   }

void ECDSA_PublicKey::set_all_values(const ECDSA_PublicKey& other)
   {
   m_param_enc = other.m_param_enc;
   m_ecdsa_core = other.m_ecdsa_core;
   m_enc_public_point = other.m_enc_public_point;
   if(other.mp_dom_pars.get())
      mp_dom_pars.reset(new EC_Domain_Params(other.domain_parameters()));

   if(other.mp_public_point.get())
      mp_public_point.reset(new PointGFp(other.public_point()));
   }

ECDSA_PublicKey::ECDSA_PublicKey(const ECDSA_PublicKey& other)
   : Public_Key(),
     EC_PublicKey(),
     PK_Verifying_wo_MR_Key()
   {
   set_all_values(other);
   }

const ECDSA_PublicKey& ECDSA_PublicKey::operator=(const ECDSA_PublicKey& rhs)
   {
   set_all_values(rhs);
   return *this;
   }

bool ECDSA_PublicKey::verify(const byte msg[], u32bit msg_len,
                             const byte sig[], u32bit sig_len) const
   {
   affirm_init();

   return m_ecdsa_core.verify(msg, msg_len, sig, sig_len);
   }

ECDSA_PublicKey::ECDSA_PublicKey(const EC_Domain_Params& dom_par,
                                 const PointGFp& public_point)
   {
   mp_dom_pars = std::auto_ptr<EC_Domain_Params>(new EC_Domain_Params(dom_par));
   mp_public_point = std::auto_ptr<PointGFp>(new PointGFp(public_point));
   m_param_enc = ENC_EXPLICIT;
   m_ecdsa_core = ECDSA_Core(*mp_dom_pars, BigInt(0), *mp_public_point);
   }

void ECDSA_PublicKey::X509_load_hook()
   {
   EC_PublicKey::X509_load_hook();
   EC_PublicKey::affirm_init();
   m_ecdsa_core = ECDSA_Core ( *mp_dom_pars, BigInt ( 0 ), *mp_public_point );
   }

u32bit ECDSA_PublicKey::max_input_bits() const
   {
   if(!mp_dom_pars.get())
      {
      throw Invalid_State("ECDSA_PublicKey::max_input_bits(): domain parameters not set");
      }
   return mp_dom_pars->get_order().bits();
   }

/*
* ECDSA_PrivateKey
*/
void ECDSA_PrivateKey::affirm_init() const // virtual
   {
   EC_PrivateKey::affirm_init();
   }

void ECDSA_PrivateKey::PKCS8_load_hook(bool generated)
   {
   EC_PrivateKey::PKCS8_load_hook(generated);
   EC_PrivateKey::affirm_init();
   m_ecdsa_core = ECDSA_Core(*mp_dom_pars, m_private_value, *mp_public_point);
   }

void ECDSA_PrivateKey::set_all_values(const ECDSA_PrivateKey& other)
   {
   m_private_value = other.m_private_value;
   m_param_enc = other.m_param_enc;
   m_ecdsa_core = other.m_ecdsa_core;
   m_enc_public_point = other.m_enc_public_point;

   if(other.mp_dom_pars.get())
      mp_dom_pars.reset(new EC_Domain_Params(other.domain_parameters()));

   if(other.mp_public_point.get())
      mp_public_point.reset(new PointGFp(other.public_point()));
   }

ECDSA_PrivateKey::ECDSA_PrivateKey(ECDSA_PrivateKey const& other)
   : Public_Key(),
     EC_PublicKey(),
     Private_Key(),
     ECDSA_PublicKey(),
     EC_PrivateKey(),
     PK_Signing_Key()
   {
   set_all_values(other);
   }

const ECDSA_PrivateKey& ECDSA_PrivateKey::operator=(const ECDSA_PrivateKey& rhs)
   {
   set_all_values(rhs);
   return *this;
   }

SecureVector<byte> ECDSA_PrivateKey::sign(const byte msg[],
                                          u32bit msg_len,
                                          RandomNumberGenerator& rng) const
   {
   affirm_init();

   const BigInt& n = mp_dom_pars->get_order();

   BigInt k;
   do
      k.randomize(rng, n.bits()-1);
   while(k >= n);

   return m_ecdsa_core.sign(msg, msg_len, k);
   }

}
