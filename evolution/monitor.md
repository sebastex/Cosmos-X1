# Evolution monitor (generation 1 done, 48.6 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c1 | 9.16 | 4 | 7.0 | 91 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| prime | 9.08 | 4 | 7.0 | 88 | (Prime) |
| c12 | 8.76 | 2 | 6.5 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c10 | 8.60 | 2 | 6.5 | 88 | plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c4 | 8.43 | 4 | 6.2 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.67306, covariance 1.0->0.76755, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998 |
| c5 | 8.39 | 2 | 6.5 | 82 | istdp_rate 50.0->56.44993, consolidation_rate 0.0->0.04656, consolidated_budget 0.5->0.55843, order_gain 4.0->1.16383, separation 3.0->4.41858 |
| c2 | 8.38 | 2 | 6.0 | 100 | istdp_rate 50.0->8.12075, istdp_target 0.02->0.01998, assembly_inhibition 0.0->0.00688, mode_tau 30.0->24.75077, separation 3.0->4.23044 |
| c11 | 8.21 | 4 | 6.2 | 78 | plastic_budget 4.0->3.35106, hetero_ltd 1.0->0.60304, covariance 1.0->0.91775, istdp_rate 50.0->46.37326, inhibition_radius3 2->3 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 6 | 24 |
| capacity | 7 | 24 |
| efficiency | 3 | 24 |
| wordpairs | 8 | 24 |
| continual | 9 | 24 |
| retention | 10 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 83% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): z>m: 1, a>w: 1, z>k: 1, m>w: 1, q>t: 1, e>t: 1, t>q: 1, w>q: 1
- Word pairs recalled wrongly (cue>recalled: times): light>house: 6, smile>candy: 5, smile>river: 4, green>dream: 4, green>water: 4, cloud>candy: 3, plant>candy: 3, apple>dream: 2

## Picture so far (all generations together)

- Brain tests so far: 48 (20 versions, generation 2).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 10 | 48 | 0 | 4 |
| capacity | 9 | 48 | 0 | 4 |
| efficiency | 3 | 48 | 0 | 4 |
| wordpairs | 8 | 48 | 0 | 4 |
| continual | 16 | 48 | 0 | 4 |
| retention | 16 | 48 | 0 | 4 |
| order | 0 | 48 | 0 | 4 |

- Old memories getting worse (all tests): wiped out +0.002, pushed aside by new memories +0.031 on average; of 16 failed 'keep old memories' tests, 13 were mainly pushed aside (interference), 3 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 309, stored but NOT recalled 75, not stored 0.
- Big-load probe (true word recall %, mean over brains): c1 32 words 69% (n=2); c1 64 words 41% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); prime 32 words 78% (n=4); prime 64 words 35% (n=4)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.52 | +0.36 | +0.43 |
| order_gain | +0.50 | +0.22 | +0.25 |
| inhibition_radius3 | -0.41 | -0.42 | -0.15 |
| presynaptic_bound | +0.39 | +0.13 | +0.21 |
| fire_threshold3 | -0.35 | -0.23 | -0.20 |
| channel_winners3 | -0.32 | -0.09 | -0.30 |
| consolidation_rate | -0.28 | -0.17 | -0.13 |
| separation | -0.26 | -0.17 | +0.00 |
| soft_bound | -0.25 | -0.23 | -0.01 |
| plastic_budget | +0.25 | +0.03 | +0.19 |
| hetero_ltd | +0.20 | +0.19 | +0.07 |
| fatigue_gain3 | +0.19 | -0.13 | +0.31 |
| covariance | -0.14 | -0.08 | -0.03 |
| consolidated_budget | +0.08 | -0.01 | +0.14 |
| assembly_inhibition | +0.08 | -0.02 | +0.12 |
| spread_plastic | +0.04 | +0.03 | +0.24 |
| mode_tau | -0.04 | -0.04 | -0.26 |
| line_recency | -0.03 | +0.02 | -0.30 |
| learning_rate | +0.02 | -0.09 | +0.03 |
| winners3 | +0.02 | +0.00 | +0.00 |
| istdp_target | +0.01 | +0.09 | -0.10 |
| order_tau | +0.01 | -0.09 | +0.10 |
| istdp_rate | +0.01 | +0.08 | +0.09 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.