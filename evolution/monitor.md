# Evolution monitor (generation 8 done, 40.7 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c36 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c48 | 9.02 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.01572, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.06565, assembly_inhibition 0.0->0.00132, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->44.6549, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c46 | 9.01 | 4 | 7.0 | 85 | learning_rate 0.04->0.05924, plastic_budget 4.0->3.19911, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->4.49026, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.54988, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.5211 |
| c1 | 9.00 | 16 | 6.8 | 91 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c51 | 8.98 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.10301 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c31 | 8.98 | 8 | 6.9 | 88 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |
| c67 | 8.93 | 2 | 7.0 | 82 | learning_rate 0.04->0.02598, plastic_budget 4.0->4.95464, soft_bound 1.0->0.887, istdp_rate 50.0->87.85399, consolidation_rate 0.0->0.00347, order_gain 4.0->4.7958, mode_tau 30.0->26.03296, winners3 3->4, separation 3.0->0.5762 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 6 | 24 |
| capacity | 5 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 10 | 24 |
| continual | 9 | 24 |
| retention | 6 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 72% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): t>a: 2, w>a: 1, z>q: 1, m>z: 1, e>a: 1, z>m: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 15, light>tiger: 7, cloud>river: 6, smile>river: 3, green>tiger: 3, cloud>candy: 3, smile>tiger: 2, cloud>water: 2

## Picture so far (all generations together)

- Brain tests so far: 216 (76 versions, generation 9).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 27 | 216 | 0 | 18 |
| capacity | 38 | 216 | 1 | 18 |
| efficiency | 8 | 216 | 0 | 18 |
| wordpairs | 35 | 216 | 1 | 18 |
| continual | 47 | 216 | 2 | 18 |
| retention | 51 | 216 | 2 | 18 |
| order | 1 | 216 | 0 | 18 |

- Old memories getting worse (all tests): wiped out -0.018, pushed aside by new memories +0.015 on average; of 47 failed 'keep old memories' tests, 36 were mainly pushed aside (interference), 11 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 1414, stored but NOT recalled 309, not stored 5.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=14); c1 64 words 40% (n=14); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 59% (n=2); c46 64 words 45% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); prime 32 words 72% (n=18); prime 64 words 32% (n=18)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.49 | +0.37 | +0.34 |
| inhibition_radius3 | -0.39 | -0.28 | -0.26 |
| order_gain | +0.36 | +0.24 | +0.19 |
| consolidation_rate | -0.34 | -0.22 | -0.17 |
| separation | -0.31 | -0.26 | -0.10 |
| hetero_ltd | +0.24 | +0.22 | +0.15 |
| presynaptic_bound | +0.22 | +0.11 | +0.10 |
| spread_plastic | +0.17 | +0.03 | +0.07 |
| soft_bound | -0.16 | -0.08 | +0.01 |
| learning_rate | -0.15 | -0.12 | -0.12 |
| plastic_budget | +0.15 | +0.09 | +0.13 |
| consolidated_budget | -0.14 | -0.02 | -0.10 |
| mode_tau | +0.09 | +0.05 | -0.05 |
| line_recency | -0.08 | +0.04 | +0.03 |
| istdp_target | +0.07 | +0.08 | -0.02 |
| order_tau | +0.07 | -0.05 | -0.05 |
| fire_threshold3 | +0.04 | +0.01 | +0.01 |
| istdp_rate | -0.04 | +0.04 | +0.05 |
| channel_winners3 | -0.03 | -0.01 | +0.00 |
| assembly_inhibition | -0.03 | -0.11 | +0.07 |
| covariance | +0.02 | +0.03 | +0.10 |
| fatigue_gain3 | +0.01 | -0.07 | +0.11 |
| winners3 | -0.00 | +0.02 | -0.05 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.