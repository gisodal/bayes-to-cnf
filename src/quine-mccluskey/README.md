# Quine-McCluskey
The Quine-McCluskey Algorithm in C++.

## Synopsis

This is a parallel implementation of the Quine-McCluskey algorithm, which produces prime implicants in sum-of-product (SOP) form. Patrick's method is used to obtain the canonical SOP form.

## Usage

> bin/quine-mccluskey -v <#variables> -o <#model1[,#model2[,...]]>

For example, given f(a,b,c) = (¬a ∧ ¬b ∧ ¬c) ∨  (¬a ∧ b ∧ ¬c) ∨ (a ∧ ¬b ∧ c) ∨ (a ∧ b ∧ c):

| Clause          | Binary | Decimal |
| :-------------: | :----: | :-----: |
| (¬a ∧ ¬b ∧ ¬c)  | 000    |  0      |
| (¬a ∧ b ∧ ¬c)   | 010    |  2      |
| (a ∧ ¬b ∧ c)    | 101    |  5      |
| (a ∧ b ∧ c)     | 111    |  7      |

Type:
  > bin/quine-mccluskey -v3 -o0,2,5,7

  ((¬a ∧ ¬c)  ∨  (a ∧ c))

## Installation

Just type:
> make

For other options, type:
> make help

