# Evolution monitor (generation 12 done, 36.2 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c99 | 9.27 | 4 | 7.0 | 94 | soft_bound 1.0->0.887, presynaptic_bound 1.0->0.86613, order_gain 4.0->5.52885, line_recency 0.6->0.54086, winners3 3->4, channel_winners3 4->3, separation 3.0->0.5762 |
| c96 | 9.25 | 2 | 7.0 | 94 | soft_bound 1.0->0.75042, hetero_ltd 1.0->0.80485, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.97447, order_gain 4.0->4.7958, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, winners3 3->4, channel_winners3 4->3, separation 3.0->4.07163 |
| c94 | 9.25 | 4 | 7.0 | 97 | learning_rate 0.04->0.03466, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.98592, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.01702, line_recency 0.6->0.66799, winners3 3->4, channel_winners3 4->3, separation 3.0->3.28214, spread_plastic 0.5->0.1191 |
| c92 | 9.16 | 2 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, covariance 1.0->0.81504, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.26742, order_tau 5.0->3.77467, mode_tau 30.0->17.29942, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->2.6822, spread_plastic 0.5->0.4135 |
| c89 | 9.12 | 4 | 7.0 | 88 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->17.38749, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.4135 |
| c105 | 9.08 | 2 | 7.0 | 88 | soft_bound 1.0->0.75042, hetero_ltd 1.0->0.80485, istdp_rate 50.0->74.60277, istdp_target 0.02->0.02106, encoding_suppression 1.0->0.97447, order_gain 4.0->4.7958, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.02495, winners3 3->5, channel_winners3 4->3, separation 3.0->4.07163 |
| c103 | 9.06 | 2 | 7.0 | 82 | plastic_budget 4.0->3.36554, presynaptic_bound 1.0->0.75759, istdp_target 0.02->0.02568, encoding_suppression 1.0->0.95112, order_gain 4.0->4.7958, order_tau 5.0->4.38934, fatigue_gain3 0.0638->0.07326, separation 3.0->0.5762, spread_plastic 0.5->0.76514 |
| c98 | 9.05 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->5.41079, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.12892, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->5.21382, spread_plastic 0.5->0.10301 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 0 | 24 |
| capacity | 1 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 1 | 24 |
| continual | 2 | 24 |
| retention | 1 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 74% of what the full letter brings (letters, 8 memories).
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 11, cloud>candy: 5, smile>river: 3, light>house: 3, cloud>water: 3, green>tiger: 2, plant>candy: 1, smile>mouse: 1

## Picture so far (all generations together)

- Brain tests so far: 312 (108 versions, generation 13).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 33 | 312 | 0 | 26 |
| capacity | 49 | 312 | 2 | 26 |
| efficiency | 12 | 312 | 1 | 26 |
| wordpairs | 43 | 312 | 1 | 26 |
| continual | 60 | 312 | 4 | 26 |
| retention | 68 | 312 | 3 | 26 |
| order | 1 | 312 | 0 | 26 |

- Old memories getting worse (all tests): wiped out -0.016, pushed aside by new memories +0.012 on average; of 60 failed 'keep old memories' tests, 44 were mainly pushed aside (interference), 16 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 2068, stored but NOT recalled 423, not stored 5.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=16); c1 64 words 39% (n=16); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c36 32 words 59% (n=2); c36 64 words 30% (n=2); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 58% (n=4); c46 64 words 40% (n=4); c48 32 words 56% (n=2); c48 64 words 34% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); c76 32 words 78% (n=2); c76 64 words 34% (n=2); c79 32 words 75% (n=4); c79 64 words 46% (n=4); c83 32 words 75% (n=2); c83 64 words 41% (n=2); c89 32 words 91% (n=2); c89 64 words 52% (n=2); c94 32 words 88% (n=2); c94 64 words 56% (n=2); c95 32 words 69% (n=2); c95 64 words 38% (n=2); c99 32 words 91% (n=2); c99 64 words 45% (n=2); prime 32 words 70% (n=26); prime 64 words 30% (n=26)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.44 | +0.32 | +0.29 |
| inhibition_radius3 | -0.37 | -0.26 | -0.22 |
| consolidation_rate | -0.33 | -0.21 | -0.14 |
| order_gain | +0.33 | +0.21 | +0.16 |
| separation | -0.22 | -0.18 | -0.04 |
| hetero_ltd | +0.20 | +0.17 | +0.12 |
| spread_plastic | +0.14 | +0.07 | +0.05 |
| learning_rate | -0.14 | -0.11 | -0.09 |
| plastic_budget | +0.12 | +0.08 | +0.11 |
| soft_bound | -0.10 | -0.03 | +0.02 |
| presynaptic_bound | +0.10 | +0.01 | +0.06 |
| winners3 | +0.09 | +0.14 | +0.04 |
| line_recency | -0.09 | +0.01 | +0.02 |
| istdp_target | +0.07 | +0.11 | -0.04 |
| consolidated_budget | -0.07 | +0.04 | -0.01 |
| mode_tau | +0.06 | -0.01 | +0.02 |
| channel_winners3 | -0.06 | -0.03 | -0.05 |
| assembly_inhibition | -0.04 | -0.12 | +0.07 |
| covariance | +0.02 | +0.02 | +0.08 |
| istdp_rate | -0.02 | +0.08 | +0.09 |
| fatigue_gain3 | -0.01 | -0.04 | +0.02 |
| order_tau | +0.01 | -0.11 | -0.10 |
| fire_threshold3 | -0.00 | -0.03 | -0.03 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.