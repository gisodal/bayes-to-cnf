# bn-to-cnf
Encode Bayesian Network into Conjunctive Normal Form.

## Synopsis
This program changes This is an implementation of the Quine-McCluskey algorithm, which produces prime implicants in sum-of-product (SOP) form. Patrick's method is used to obtain the canonical SOP form.

## Usage
The program takes as i
  > ./bn-to-cnf -i <HUGIN FILE> [option] [...]

  Options:
     -c: Constraints are supressed
     -e: Equal probabilities is encoded
     -d: Determinism is encoded
     -s: Symplify encoding
     -i: Input file
     -w: Write CNF in DIMACS format to file
     -p: Print stats to stdout
     -h: Help


For example:
> bin/quine-mccluskey -v3 -o0,2,5,7

((¬a ∧ ¬c)  ∨  (a ∧ c))

## Installation

Just type:
> make

for other option, type:
> make help

