#! /bin/sh


echo '# Copyright (c) 2007-2012 Nicta, OML Contributors <oml-user@lists.nicta.com.au>'
echo '#'
echo '# Licensing information can be found in file COPYING.'
echo ''

(
  # Pre-git authors
  # Some authors of "ORBIT Measurements Framework and Library (OML): Motivations, Design, Implementation, and Features" (others in Git log)
  echo 'Manpreet Singh'
  echo 'Ivan Seskar'
  echo 'Pandurang Kama'
  git log | sed -n -e 's/^Author: *//p'
) \
  | sed -e 's/ *<.*>.*//' \
  | sort \
  | uniq
