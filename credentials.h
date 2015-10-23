/*
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef EXAMPLE_CREDENTIALS_MANAGER_H__
#define EXAMPLE_CREDENTIALS_MANAGER_H__

#include <botan/pkcs8.h>
#include <botan/credentials_manager.h>
#include <botan/x509self.h>
#include <iostream>
#include <fstream>
#include <memory>

using namespace Botan;

inline bool value_exists(const std::vector<std::string>& vec,
                         const std::string& val)
   {
   for(size_t i = 0; i != vec.size(); ++i)
      if(vec[i] == val)
         return true;
   return false;
   }

class Basic_Credentials_Manager : public Credentials_Manager
   {
   public:
      Basic_Credentials_Manager()
         {
         load_certstores();
         }

      Basic_Credentials_Manager(RandomNumberGenerator& rng,
                                const std::string& server_crt,
                                const std::string& server_key)
         {
         Certificate_Info cert;

         cert.key.reset(PKCS8::load_key(server_key, rng));

         DataSource_Stream in(server_crt);
         while(!in.end_of_data())
            {
            try
               {
               cert.certs.push_back(X509_Certificate(in));
               }
            catch(std::exception& e)
               {

               }
            }

         // TODO: attempt to validate chain ourselves

         m_creds.push_back(cert);
         }

      void load_certstores()
         {
         try
            {
            // TODO: make path configurable
            const std::vector<std::string> paths = { "/usr/share/ca-certificates" };

            for(auto&& path : paths)
               {
               std::shared_ptr<Certificate_Store> cs(new Certificate_Store_In_Memory(path));
               m_certstores.push_back(cs);
               }
            }
         catch(std::exception& e)
            {
            //std::cout << e.what() << "\n";
            }
         }

      std::vector<Botan::Certificate_Store*>
      trusted_certificate_authorities(const std::string& type,
                                      const std::string& /*hostname*/) override
         {
         std::vector<Botan::Certificate_Store*> v;

         // don't ask for client certs
         if(type == "tls-server")
            return v;

         for(auto&& cs : m_certstores)
            v.push_back(cs.get());

         return v;
         }

      void verify_certificate_chain(
         const std::string& type,
         const std::string& purported_hostname,
         const std::vector<X509_Certificate>& cert_chain) override
         {
         try
            {
            Credentials_Manager::verify_certificate_chain(type,
                                                          purported_hostname,
                                                          cert_chain);
            }
         catch(std::exception& e)
            {
            std::cout << e.what() << std::endl;
            //throw;
            }
         }

      std::vector<X509_Certificate> cert_chain(
         const std::vector<std::string>& algos,
         const std::string& type,
         const std::string& hostname) override
         {
         BOTAN_UNUSED(type);

         for(auto&& i : m_creds)
            {
            if(std::find(algos.begin(), algos.end(), i.key->algo_name()) == algos.end())
               continue;

            if(hostname != "" && !i.certs[0].matches_dns_name(hostname))
               continue;

            return i.certs;
            }

         return std::vector<X509_Certificate>();
         }

      Private_Key* private_key_for(const X509_Certificate& cert,
                                   const std::string& /*type*/,
                                   const std::string& /*context*/) override
         {
         for(auto&& i : m_creds)
            {
            if(cert == i.certs[0])
               return i.key.get();
            }

         return nullptr;
         }

   private:
      struct Certificate_Info
         {
         std::vector<X509_Certificate> certs;
         std::shared_ptr<Private_Key> key;
         };

      std::vector<Certificate_Info> m_creds;
      std::vector<std::shared_ptr<Certificate_Store>> m_certstores;
   };

#endif
