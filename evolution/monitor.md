# Evolution monitor (generation 5 done, 44.1 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c45 | 9.21 | 2 | 7.0 | 94 | plastic_budget 4.0->4.62789, soft_bound 1.0->0.85036, presynaptic_bound 1.0->0.96957, istdp_rate 50.0->59.06565, consolidation_rate 0.0->0.02282, order_gain 4.0->3.81883, fire_threshold3 0.0256->0.03696, winners3 3->4, separation 3.0->2.45408, spread_plastic 0.5->0.59232 |
| c1 | 9.20 | 12 | 7.0 | 92 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c31 | 9.18 | 6 | 7.0 | 90 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |
| c44 | 9.12 | 2 | 7.0 | 94 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->3.77761, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.73329, winners3 3->4 |
| c46 | 9.09 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->3.19911, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->4.49026, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.54988, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.5211 |
| c30 | 9.08 | 4 | 7.0 | 88 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->2.85837, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.9167 |
| c36 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c48 | 9.02 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.01572, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.06565, assembly_inhibition 0.0->0.00132, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->44.6549, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 1 | 24 |
| capacity | 1 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 0 | 24 |
| continual | 2 | 24 |
| retention | 3 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 69% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): z>q: 1, z>w: 1, a>t: 1, q>t: 1, t>k: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 14, plant>candy: 1, smile>river: 1, cloud>water: 1, cloud>mouse: 1, cloud>candy: 1

## Picture so far (all generations together)

- Brain tests so far: 144 (52 versions, generation 6).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 17 | 144 | 0 | 12 |
| capacity | 23 | 144 | 0 | 12 |
| efficiency | 6 | 144 | 0 | 12 |
| wordpairs | 17 | 144 | 1 | 12 |
| continual | 29 | 144 | 1 | 12 |
| retention | 32 | 144 | 1 | 12 |
| order | 1 | 144 | 0 | 12 |

- Old memories getting worse (all tests): wiped out -0.012, pushed aside by new memories +0.016 on average; of 29 failed 'keep old memories' tests, 24 were mainly pushed aside (interference), 5 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 960, stored but NOT recalled 190, not stored 2.
- Big-load probe (true word recall %, mean over brains): c1 32 words 72% (n=10); c1 64 words 42% (n=10); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 34% (n=2); c30 64 words 6% (n=2); c31 32 words 80% (n=4); c31 64 words 49% (n=4); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); prime 32 words 76% (n=12); prime 64 words 34% (n=12)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| inhibition_radius3 | -0.55 | -0.42 | -0.34 |
| order_gain | +0.52 | +0.33 | +0.24 |
| consolidation_rate | -0.46 | -0.29 | -0.21 |
| encoding_suppression | +0.42 | +0.29 | +0.29 |
| separation | -0.30 | -0.28 | -0.09 |
| presynaptic_bound | +0.30 | +0.16 | +0.15 |
| channel_winners3 | -0.23 | -0.15 | -0.12 |
| plastic_budget | +0.21 | +0.06 | +0.17 |
| hetero_ltd | +0.20 | +0.21 | +0.06 |
| order_tau | -0.19 | -0.22 | -0.14 |
| winners3 | +0.17 | +0.23 | +0.08 |
| fatigue_gain3 | +0.15 | -0.01 | +0.17 |
| mode_tau | +0.14 | +0.09 | -0.06 |
| istdp_rate | +0.13 | +0.20 | +0.11 |
| fire_threshold3 | -0.11 | -0.12 | -0.09 |
| line_recency | +0.10 | +0.12 | +0.04 |
| soft_bound | -0.09 | -0.02 | +0.07 |
| consolidated_budget | +0.08 | +0.16 | -0.01 |
| covariance | +0.06 | +0.05 | +0.14 |
| spread_plastic | +0.02 | -0.09 | +0.01 |
| istdp_target | +0.02 | +0.09 | -0.04 |
| learning_rate | +0.02 | +0.03 | +0.04 |
| assembly_inhibition | -0.01 | -0.08 | +0.10 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.