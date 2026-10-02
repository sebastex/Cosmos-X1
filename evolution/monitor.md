# Evolution monitor (generation 11 done, 37.3 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c94 | 9.32 | 2 | 7.0 | 100 | learning_rate 0.04->0.03466, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.98592, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.01702, line_recency 0.6->0.66799, winners3 3->4, channel_winners3 4->3, separation 3.0->3.28214, spread_plastic 0.5->0.1191 |
| c95 | 9.29 | 2 | 7.0 | 94 | order_gain 4.0->4.7958, order_tau 5.0->4.38934, fatigue_gain3 0.0638->0.07326, separation 3.0->0.5762, spread_plastic 0.5->0.74239 |
| c99 | 9.27 | 2 | 7.0 | 94 | soft_bound 1.0->0.887, presynaptic_bound 1.0->0.86613, order_gain 4.0->5.52885, line_recency 0.6->0.54086, winners3 3->4, channel_winners3 4->3, separation 3.0->0.5762 |
| c96 | 9.25 | 2 | 7.0 | 94 | soft_bound 1.0->0.75042, hetero_ltd 1.0->0.80485, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.97447, order_gain 4.0->4.7958, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, winners3 3->4, channel_winners3 4->3, separation 3.0->4.07163 |
| c92 | 9.16 | 2 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, covariance 1.0->0.81504, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.26742, order_tau 5.0->3.77467, mode_tau 30.0->17.29942, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->2.6822, spread_plastic 0.5->0.4135 |
| c89 | 9.12 | 4 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->17.38749, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.4135 |
| c98 | 9.05 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->5.41079, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.12892, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->5.21382, spread_plastic 0.5->0.10301 |
| c93 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->77.12281, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, assembly_inhibition 0.0->0.00143, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, separation 3.0->3.5594, spread_plastic 0.5->0.10301 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 0 | 24 |
| capacity | 1 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 1 | 24 |
| continual | 0 | 24 |
| retention | 1 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 75% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): e>t: 1, m>a: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 14, plant>river: 2, cloud>candy: 2, cloud>water: 2, light>house: 1

## Picture so far (all generations together)

- Brain tests so far: 288 (100 versions, generation 12).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 33 | 288 | 0 | 24 |
| capacity | 48 | 288 | 2 | 24 |
| efficiency | 11 | 288 | 1 | 24 |
| wordpairs | 42 | 288 | 1 | 24 |
| continual | 58 | 288 | 3 | 24 |
| retention | 67 | 288 | 3 | 24 |
| order | 1 | 288 | 0 | 24 |

- Old memories getting worse (all tests): wiped out -0.017, pushed aside by new memories +0.013 on average; of 58 failed 'keep old memories' tests, 44 were mainly pushed aside (interference), 14 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 1905, stored but NOT recalled 394, not stored 5.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=16); c1 64 words 39% (n=16); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c36 32 words 59% (n=2); c36 64 words 30% (n=2); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 58% (n=4); c46 64 words 40% (n=4); c48 32 words 56% (n=2); c48 64 words 34% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); c76 32 words 78% (n=2); c76 64 words 34% (n=2); c79 32 words 75% (n=4); c79 64 words 46% (n=4); c83 32 words 75% (n=2); c83 64 words 41% (n=2); c89 32 words 91% (n=2); c89 64 words 52% (n=2); prime 32 words 70% (n=24); prime 64 words 30% (n=24)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.45 | +0.33 | +0.30 |
| inhibition_radius3 | -0.39 | -0.28 | -0.23 |
| order_gain | +0.34 | +0.23 | +0.15 |
| consolidation_rate | -0.33 | -0.20 | -0.13 |
| separation | -0.22 | -0.19 | -0.03 |
| hetero_ltd | +0.21 | +0.19 | +0.13 |
| presynaptic_bound | +0.19 | +0.07 | +0.09 |
| plastic_budget | +0.13 | +0.08 | +0.12 |
| spread_plastic | +0.13 | +0.06 | +0.03 |
| learning_rate | -0.12 | -0.09 | -0.08 |
| soft_bound | -0.11 | -0.03 | +0.02 |
| consolidated_budget | -0.08 | +0.02 | -0.00 |
| line_recency | -0.07 | +0.02 | +0.04 |
| winners3 | +0.07 | +0.11 | +0.03 |
| istdp_target | +0.06 | +0.10 | -0.05 |
| mode_tau | +0.06 | -0.01 | +0.00 |
| channel_winners3 | -0.04 | +0.00 | -0.04 |
| assembly_inhibition | -0.03 | -0.11 | +0.07 |
| covariance | +0.03 | +0.03 | +0.08 |
| istdp_rate | -0.02 | +0.08 | +0.10 |
| order_tau | +0.02 | -0.10 | -0.10 |
| fire_threshold3 | +0.01 | -0.01 | -0.03 |
| fatigue_gain3 | +0.00 | -0.03 | +0.03 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.