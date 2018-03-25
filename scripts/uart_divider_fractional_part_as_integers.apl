⍝ Fractional values from MSP432P4xx technical reference manual, Rev. H,
⍝ table 24-4 UCBRSx Settings for Fractional Portion of N=f_BRCLK/Baud Rate
frac_part ← 0.0000 0.0529 0.0715 0.0835 0.1001 0.1252 0.1430 0.1670 0.2147 0.2224 0.2503 0.3000 0.3335 0.3575 0.3753 0.4003 0.4286 0.4378 0.5002 0.5715 0.6003 0.6254 0.6432 0.6667 0.7001 0.7147 0.7503 0.7861 0.8004 0.8333 0.8464 0.8572 0.8751 0.9004 0.9170 0.9288
⍝ We want the fractional part to be a 16-bit unsigned integer, so multiply by 2*16
⌊(2*16)×frac_part