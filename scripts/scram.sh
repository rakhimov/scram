# bash completion for scram
# run 'source scram.sh' or put this file in /etc/bash_completion.d/

__parse_desc_options()
{
  local option option2 i

  option=
  option2=

  for i in $1; do
      case $i in
          --?*) option=$i ; continue ;;
          -?*)  [[ $option ]] || option2=$i ;;
          *)    continue ;;
      esac
  done

  [[ $option ]] || [[ $option2 ]] || return 0

  printf '%s\n' $option

  printf '%s\n' $option2
}

_parse_desc()
{
  local line
  scram --help | while read -r line; do

      [[ $line == *([ $'\t'])-* ]] || continue
      __parse_desc_options "$line"

  done
}

_scram()
{
  local cur prev words cword
  _init_completion -n = || return

  case "$prev" in
    -o|--output-path|--input-files|--config-file)
      _filedir
      return
      ;;
    -l|--limit-order|-s|--num-sums|--cut-off|--mission-time|--num-trials)
      # argument required but no completions available
      return
      ;;
    --probability)
      COMPREPLY=( $( compgen -W '0 1' -- "$cur" ) )
      return
      ;;
    --importance)
      COMPREPLY=( $( compgen -W '0 1' -- "$cur" ) )
      return
      ;;
    --ccf)
      COMPREPLY=( $( compgen -W '0 1' -- "$cur" ) )
      return
      ;;
    --uncertainty)
      COMPREPLY=( $( compgen -W '0 1' -- "$cur" ) )
      return
      ;;
  esac

  if [[ "$cur" == -* ]]; then
    COMPREPLY=( $(compgen -W '$( _parse_desc )' -- ${cur}) )
    [[ $COMPREPLY == *= ]] && compopt -o nospace
    [[ $COMPREPLY ]] && return
  fi

  _filedir
} &&
complete -F _scram scram
