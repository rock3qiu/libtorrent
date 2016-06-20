/*

Copyright (c) 2007-2016, Un Shyam & Arvid Norberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#if !defined(TORRENT_DISABLE_ENCRYPTION) && !defined(TORRENT_DISABLE_EXTENSIONS)

#ifndef TORRENT_PE_CRYPTO_HPP_INCLUDED
#define TORRENT_PE_CRYPTO_HPP_INCLUDED

#include "libtorrent/config.hpp"

#include "libtorrent/aux_/disable_warnings_push.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include "libtorrent/aux_/disable_warnings_pop.hpp"

#include "libtorrent/receive_buffer.hpp"
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/extensions.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/aux_/array_view.hpp"

#include <list>
#include <array>
#include <cstdint>

namespace libtorrent
{
	namespace mp = boost::multiprecision;

	using key_t = mp::number<mp::cpp_int_backend<768, 768, mp::unsigned_magnitude, mp::unchecked, void> >;

	// RC4 state from libtomcrypt
	struct rc4 {
		int x, y;
		std::array<std::uint8_t, 256> buf;
	};

	void TORRENT_EXTRA_EXPORT rc4_init(const unsigned char* in, unsigned long len, rc4 *state);
	unsigned long TORRENT_EXTRA_EXPORT rc4_encrypt(unsigned char *out, unsigned long outlen, rc4 *state);

	// TODO: 3 dh_key_exchange should probably move into its own file
	class TORRENT_EXTRA_EXPORT dh_key_exchange
	{
	public:
		dh_key_exchange();
		bool good() const { return true; }

		// Get local public key
		key_t const& get_local_key() const { return m_dh_local_key; }

		// read remote_pubkey, generate and store shared secret in
		// m_dh_shared_secret.
		void compute_secret(std::uint8_t const* remote_pubkey);
		void compute_secret(key_t const& remote_pubkey);

		key_t const& get_secret() const { return m_dh_shared_secret; }

		sha1_hash const& get_hash_xor_mask() const { return m_xor_mask; }

	private:

		int get_local_key_size() const
		{ return sizeof(m_dh_local_key); }

		key_t m_dh_local_key;
		key_t m_dh_local_secret;
		key_t m_dh_shared_secret;
		sha1_hash m_xor_mask;
	};

	struct encryption_handler
	{
		std::tuple<int, aux::array_view<boost::asio::const_buffer>>
		encrypt(aux::array_view<boost::asio::mutable_buffer> iovec);

		int decrypt(crypto_receive_buffer& recv_buffer
			, std::size_t& bytes_transferred);

		bool switch_send_crypto(boost::shared_ptr<crypto_plugin> crypto
			, int pending_encryption);

		void switch_recv_crypto(boost::shared_ptr<crypto_plugin> crypto
			, crypto_receive_buffer& recv_buffer);

		bool is_send_plaintext() const
		{
			return m_send_barriers.empty() || m_send_barriers.back().next != INT_MAX;
		}

		bool is_recv_plaintext() const
		{
			return m_dec_handler.get() == NULL;
		}

	private:
		struct barrier
		{
			barrier(boost::shared_ptr<crypto_plugin> plugin, int n)
				: enc_handler(plugin), next(n) {}
			boost::shared_ptr<crypto_plugin> enc_handler;
			// number of bytes to next barrier
			int next;
		};
		std::list<barrier> m_send_barriers;
		boost::shared_ptr<crypto_plugin> m_dec_handler;
	};

	struct TORRENT_EXTRA_EXPORT rc4_handler : crypto_plugin
	{
	public:
		rc4_handler();

		// Input keys must be 20 bytes
		// TODO: 4 use uint768_t here instead of pointer + length
		void set_incoming_key(unsigned char const* key, int len) override;
		void set_outgoing_key(unsigned char const* key, int len) override;

		std::tuple<int, aux::array_view<boost::asio::const_buffer>>
		encrypt(aux::array_view<boost::asio::mutable_buffer> buf) override;

		void decrypt(aux::array_view<boost::asio::mutable_buffer> buf
			, int& consume
			, int& produce
			, int& packet_size) override;

	private:
		rc4 m_rc4_incoming;
		rc4 m_rc4_outgoing;

		// determines whether or not encryption and decryption is enabled
		bool m_encrypt;
		bool m_decrypt;
	};

} // namespace libtorrent

#endif // TORRENT_PE_CRYPTO_HPP_INCLUDED
#endif // TORRENT_DISABLE_ENCRYPTION

