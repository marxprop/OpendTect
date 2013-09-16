<?php

/**
 * Enforces basic text file rules.
 *
 * @group linter
 */
final class TextLinter extends ArcanistLinter {

  const LINT_DOS_NEWLINE            = 1;
  const LINT_TAB_LITERAL            = 2;
  const LINT_LINE_WRAP              = 3;
  const LINT_EOF_NEWLINE            = 4;
  const LINT_BAD_CHARSET            = 5;
  const LINT_TRAILING_WHITESPACE    = 6;
  const LINT_NO_COMMIT              = 7;

  private $maxLineLength = 80;

  public function getLinterPriority() {
    return 0.5;
  }

  public function setMaxLineLength($new_length) {
    $this->maxLineLength = $new_length;
    return $this;
  }

  public function getLinterName() {
    return 'TXT';
  }

  public function getLinterConfigurationName() {
    return 'text';
  }

  public function getLintSeverityMap() {
    return array(
      self::LINT_TRAILING_WHITESPACE => ArcanistLintSeverity::SEVERITY_AUTOFIX,
    );
  }

  public function getLintNameMap() {
    return array(
      self::LINT_DOS_NEWLINE          => pht('DOS Newlines'),
      self::LINT_TAB_LITERAL          => pht('Tab Literal'),
      self::LINT_LINE_WRAP            => pht('Line Too Long'),
      self::LINT_EOF_NEWLINE          => pht('File Does Not End in Newline'),
      self::LINT_BAD_CHARSET          => pht('Bad Charset'),
      self::LINT_TRAILING_WHITESPACE  => pht('Trailing Whitespace'),
      self::LINT_NO_COMMIT            => pht('Explicit %s', '@no'.'commit'),
    );
  }

  public function lintPath($path) {
    if (!strlen($this->getData($path))) {
      // If the file is empty, don't bother; particularly, don't require
      // the user to add a newline.
      return;
    }

    //$this->lintNewlines($path);
//
    //if ($this->didStopAllLinters()) {
      //return;
    //}

    $this->lintCharset($path);

    if ($this->didStopAllLinters()) {
      return;
    }

    $this->lintEOFNewline($path);
    $this->lintTrailingWhitespace($path);

    $iscmake = strpos( $path, ".cmake" )!==false;

    if (!$iscmake)
      $this->lintLineLength($path);

    if ($this->getEngine()->getCommitHookMode()) {
      $this->lintNoCommit($path);
    }
  }

  protected function lintNewlines($path) {
    $pos = strpos($this->getData($path), "\r");
    if ($pos !== false) {
      $this->raiseLintAtOffset(
        $pos,
        self::LINT_DOS_NEWLINE,
        'You must use ONLY Unix linebreaks ("\n") in source code.',
        "\r");
      if ($this->isMessageEnabled(self::LINT_DOS_NEWLINE)) {
        $this->stopAllLinters();
      }
    }
  }

  private function tab_expand($text) {
    $tab_stop = 8;
    $res = "";
    for ( $idx=0; $idx<strlen($text); $idx++ ) {
      if ( $text[$idx]=="\t" ) {
	$curpos = strlen($res);
	$nrtabs = (int) ($curpos/$tab_stop);
	$stop = ($nrtabs+1)*$tab_stop;
	$nrspaces = $stop-$curpos;
	for ( $idy=$nrspaces-1; $idy>=0; $idy-- ) {
	  $res = $res." ";
	}
      } 
      else if ( $text[$idx]!=="\r" ) {
	$res = $res.$text[$idx];
      }
     
    }

    return $res;
  }

  protected function lintLineLength($path) {
    $lines = explode("\n", $this->getData($path));

    $width = $this->maxLineLength;
    foreach ($lines as $line_idx => $line) {

      $expandedline = $this->tab_expand( $line );
      if (strlen($expandedline) > $width) {

	$isrcs = strpos( $expandedline, '$Id' );

	if ( $isrcs==false ) {
	    $this->raiseLintAtLine(
	      $line_idx + 1,
	      1,
	      self::LINT_LINE_WRAP,
	      'This line is '.number_format(strlen($expandedline)).
	      ' characters long, '.
	      'but the convention is '.$width.' characters.',
	      $expandedline);
	}
      }
    }
  }

  protected function lintEOFNewline($path) {
    $data = $this->getData($path);
    if (!strlen($data) || $data[strlen($data) - 1] != "\n") {
      $this->raiseLintAtOffset(
        strlen($data),
        self::LINT_EOF_NEWLINE,
        "Files must end in a newline.",
        '',
        "\n");
    }
  }

  protected function lintCharset($path) {
    $data = $this->getData($path);

    $matches = null;
    $bad = '[^\x09\x0A\x0D\x20-\x7E]';
    $preg = preg_match_all(
      "/{$bad}(.*{$bad})?/",
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_BAD_CHARSET,
        'Source code should contain only ASCII bytes with ordinal decimal '.
        'values between 32 and 126 inclusive, plus linefeed. Do not use UTF-8 '.
        'or other multibyte charsets.',
        $string);
    }

    if ($this->isMessageEnabled(self::LINT_BAD_CHARSET)) {
      $this->stopAllLinters();
    }
  }

  protected function lintTrailingWhitespace($path) {
    $data = $this->getData($path);

    $matches = null;
    $preg = preg_match_all(
      '/[ \t]+$/m',
      $data,
      $matches,
      PREG_OFFSET_CAPTURE);

    if (!$preg) {
      return;
    }

    foreach ($matches[0] as $match) {
      list($string, $offset) = $match;
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_TRAILING_WHITESPACE,
        'This line contains trailing whitespace. Consider setting up your '.
          'editor to automatically remove trailing whitespace, you will save '.
          'time.',
        $string,
        '');
    }
  }

  private function lintNoCommit($path) {
    $data = $this->getData($path);

    $deadly = '@no'.'commit';

    $offset = strpos($data, $deadly);
    if ($offset !== false) {
      $this->raiseLintAtOffset(
        $offset,
        self::LINT_NO_COMMIT,
        'This file is explicitly marked as "'.$deadly.'", which blocks '.
        'commits.',
        $deadly);
    }
  }


}
