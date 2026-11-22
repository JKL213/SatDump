#include "viterbi_1_2.h"
#include <algorithm>
#include <cstring>
#include "utils.h"

#define ST_IDLE 0
#define ST_SYNCED 1

namespace viterbi
{
    Viterbi1_2::Viterbi1_2(float ber_threshold, int max_outsync, int buffer_size, std::vector<phase_t> phases, bool check_iq_swap)
        : d_ber_thresold(ber_threshold),
          d_max_outsync(max_outsync),
          d_check_iq_swap(check_iq_swap),
          d_buffer_size(buffer_size),
          d_phases_to_check(phases),
          d_state(ST_IDLE),
          cc_decoder_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
          cc_encoder_ber(TEST_BITS_LENGTH / 2, 7, 2, {79, 109}),
          cc_decoder(buffer_size / 2, 7, 2, {79, 109})

    {
        rot_buffer = new int8_t[d_buffer_size * 2];
        unsigned_buffer = new uint8_t[d_buffer_size * 2];
        soft_buffer = new uint8_t[d_buffer_size * 2 + TAIL_SYMBOLS + 4];
        output_buffer = new uint8_t[d_buffer_size * 2];
        memset(d_tail_carry, 128, TAIL_SYMBOLS);

        for (int s = 0; s < 2; s++)
            for (int i = 0; i < 2; i++)
                for (int y = 0; y < 4; y++)
                    d_bers[s][y][i] = 10;
    }

    Viterbi1_2::~Viterbi1_2()
    {
        delete[] rot_buffer;
        delete[] unsigned_buffer;
        delete[] soft_buffer;
        delete[] output_buffer;
    }

    float Viterbi1_2::get_ber(uint8_t *raw, uint8_t *rencoded, int len)
    {
        float errors = 0, total = 0;
        // Skip the first 6 bit pairs (12 symbols) where CCEncoder's shift register fills.
        for (int i = TAIL_SYMBOLS; i < len; i++)
        {
            if (raw[i] != 128)
            {
                errors += (raw[i] > 127) != rencoded[i];
                total++;
            }
        }

        return total > 0 ? (errors / total) * 2.5f : 10.0f;
    }

    int Viterbi1_2::work(int8_t *input, int size, uint8_t *output)
    {
        if (d_state == ST_IDLE) // Search for a lock
        {
            d_ber = 10;
            const int test_len = std::min(size, TEST_BITS_LENGTH + TAIL_SYMBOLS + 2) & ~1;
            for (int s = 0; s < (d_check_iq_swap ? 2 : 1); s++)
            {
                for (phase_t phase : d_phases_to_check)
                {
                    memset(ber_test_buffer, 0, sizeof(ber_test_buffer));
                    memset(ber_soft_buffer, 128, sizeof(ber_soft_buffer));
                    memcpy(ber_test_buffer, input, test_len);                               // Copy over small buffer
                    rotate_soft(ber_test_buffer, test_len, PHASE_0, s);                     // IQ Swap
                    rotate_soft(ber_test_buffer, test_len, phase, false);                   // Phase shift
                    signed_soft_to_unsigned(ber_test_buffer, ber_soft_buffer, test_len);    // Convert to softs for the viterbi

                    for (int shift = 0; shift < 2; shift++) // Test 2 puncturing shifts
                    {
                        cc_decoder_ber.reset();
                        cc_encoder_ber.reset();
                        cc_decoder_ber.work(ber_soft_buffer + shift, ber_decoded_buffer); // Decode....
                        cc_encoder_ber.work(ber_decoded_buffer, ber_encoded_buffer);      // ....then reencode for comparison

                        d_bers[s][phase][shift] = get_ber(ber_soft_buffer + shift, ber_encoded_buffer, TEST_BITS_LENGTH); // Compute BER between initial buffer and re-encoded

                        if ((d_ber == 10 && d_bers[s][phase][shift] < d_ber_thresold) || // Check for a lock initially
                            (d_ber < 10 && d_bers[s][phase][shift] < d_ber))             // And if we find a lower BER, prefer it
                        {
                            d_ber = d_bers[s][phase][shift]; // Set current BER
                            d_iq_swap = s;
                            d_state = ST_SYNCED;      // Set the decoder state to SYNCED so we start decoding
                            d_phase = (phase_t)phase; // Set current phase
                            d_shift = shift;          // Set current puncturing shift
                            d_invalid = 0;            // Reset invalid BER count
                            d_has_carry = false;
                            d_carry_symbol = 128;
                            memset(d_tail_carry, 128, TAIL_SYMBOLS);
                            memset(soft_buffer, 128, d_buffer_size * 2 + TAIL_SYMBOLS + 4);
                            cc_decoder.reset();
                        }
                    }
                }
            }
        }

        int out_n = 0; // Output bytes count

        if (d_state == ST_SYNCED) // Decode
        {
            memcpy(rot_buffer, input, size);
            rotate_soft(rot_buffer, size, PHASE_0, d_iq_swap);
            rotate_soft(rot_buffer, size, d_phase, false);            // Phase shift
            signed_soft_to_unsigned(rot_buffer, unsigned_buffer, size); // Soft conversion

            uint8_t *aligned = unsigned_buffer;
            if (d_shift)
            {
                // Preserve the odd trailing symbol across buffer boundaries instead of dropping it.
                uint8_t next_carry = unsigned_buffer[size - 1];
                memmove(unsigned_buffer + 1, unsigned_buffer, size - 1);
                unsigned_buffer[0] = d_has_carry ? d_carry_symbol : 128;
                d_carry_symbol = next_carry;
                d_has_carry = true;
            }

            // Prepend the 12-symbol (6-bit) tail from the previous block so CCDecoder
            // always has a real 6-bit lookahead tail in soft_buffer[size .. size + 11]
            // without dropping or duplicating any bits across block boundaries.
            memcpy(soft_buffer, d_tail_carry, TAIL_SYMBOLS);
            memcpy(soft_buffer + TAIL_SYMBOLS, aligned, size);
            if (size >= TAIL_SYMBOLS)
                memcpy(d_tail_carry, aligned + size - TAIL_SYMBOLS, TAIL_SYMBOLS);
            cc_decoder.work(soft_buffer, output);
            out_n = size / 2;

            cc_encoder_ber.work(output, ber_encoded_buffer);                  // Re-encoded for a BER check
            d_ber = get_ber(soft_buffer, ber_encoded_buffer, TEST_BITS_LENGTH); // Compute BER

            if (d_ber > d_ber_thresold) // Check current BER
            {
                d_invalid++;
                if (d_invalid > d_max_outsync) // If we get over out max unsynced thresold...
                    d_state = ST_IDLE;         // ...reset the decoder
            }
            else
            {
                d_invalid = 0; // Otherwise, reset current count
            }
        }

        return out_n;
    }

    float Viterbi1_2::ber()
    {
        if (d_state == ST_SYNCED)
            return d_ber;
        else
        {
            float ber = 10;
            for (int s = 0; s < (d_check_iq_swap ? 2 : 1); s++)
                for (phase_t phase : d_phases_to_check)
                    for (int o = 0; o < 2; o++)
                        if (ber > d_bers[s][phase][o])
                            ber = d_bers[s][phase][o];
            return ber;
        }
    }

    int Viterbi1_2::getState()
    {
        return d_state;
    }
}
