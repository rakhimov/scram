#! /usr/bin/python

import random
import Queue

import argparse as ap

# Global variables to be used for counting and naming primary and intermediate
# events for a fault tree
nprime = 0  # number of primary events
ninter = 0  # number of intermediate events

class TopEvent():
    def __init__(self):
        self.id = "TopEvent"  # default name for a top event
        self.p_children = set()  # children that are primary events
        self.i_children = set()  # children that are intermediate events
        self.gate = ""

class InterEvent():
    def __init__(self):
        global ninter
        ninter = ninter + 1
        self.id = "E" + str(ninter)
        self.p_children = set()
        self.i_children = set()
        self.gate = ""

class PrimaryEvent():
    def __init__(self):
        global nprime
        nprime = nprime + 1
        self.id = "P" + str(nprime)
        self.type = "Basic"  # all events in this tree are basic
        self.prob = 0  # probability

def main():
    description = "A module for creating a fault tree of an arbitrary size."

    parser = ap.ArgumentParser(description=description)

    seed = "the seed of a random number generator"
    parser.add_argument("-s", "--seed", type=int, help=seed, default=123)

    nprimary = "the number of unique primary events"
    parser.add_argument("-p", "--nprimary", type=int, help=nprimary,
                        choices=range(2, 1000000), default=10)

    nchildren = "the average number of children per intermediate event"
    parser.add_argument("-c", "--nchildren", type=int, help=nchildren,
                        choices=range(2, 100), default=3)

    ratio = "primary events to intermediate events ratio per new node"
    parser.add_argument("-r", "--ratio", type=float, help=ratio,
                        choices=range(1, 1000), default=2)

    reuse = "approximate percentage of repeated primary event in the tree"
    parser.add_argument("-u", "--reuse", type=float, help=reuse,
                        choices=range(0, 1), default=0.1)

    maxprob = "maximum probability for primary events"
    parser.add_argument("-x", "--maxprob", type=float, help=maxprob,
                        choices=range(0, 1), default=0.1)

    minprob = "minimum probability for primary events"
    parser.add_argument("-m", "--minprob", type=float, help=minprob,
                        choices=range(0, 1), default=0.001)

    topprime = "minimal number of primary events for a root node"
    parser.add_argument("--topprime", type=int, help=topprime,
                        choices=range(0, 1000000), default=0)

    ctop = "minimal number of children for a root node"
    parser.add_argument("-n", "--ctop", type=int, help=ctop,
                        choices=range(2, 1000000), default=0)

    out = "name of a file without extension to write the fault tree"
    parser.add_argument("-o", "--out", help=out, default="fta_tree")

    args = parser.parse_args()

    # check validity of arguments
    if args.maxprob < args.minprob:
        raise args.ArgumentTypeError("Max probability < Min probability")

    if args.topprime > args.nprimary:
        raise args.ArgumentTypeError("Topprime > number of total primary events")

    if args.topprime > args.ctop:
        raise args.ArgumentTypeError("Topprime > number of children for top")

    # set the seed for this tree generator
    random.seed(args.seed)

    # container for created primary events
    primary_vec = []

    # supported types of gates
    types = ["OR", "AND"]

    # minimum number of children per intermediate event
    min_children = 2

    # maximum number of children
    max_children = args.nchildren * 2 - min_children

    # Tree generation
    # start with a top event
    top_event = TopEvent()
    top_event.id = "TopEvent"
    top_event.gate = random.choice(types)
    child_size = random.randint(min_children, max_children)

    # Configuring child size for the top event
    if args.ctop != 0:
        child_size = args.ctop
    elif child_size < args.topprime:
        child_size = args.topprime

    # container for not yet initialized intermediate events
    inters_queue = Queue.Queue()

    def create_inter(parent):
        inter_event = InterEvent()
        inter_event.gate = random.choice(types)
        parent.i_children.add(inter_event)
        inters_queue.put(inter_event)

    def create_primary(parent):
        prime_event = PrimaryEvent()
        prime_event.prob = random.uniform(args.minprob, args.maxprob)
        primary_vec.append(prime_event)
        parent.p_children.add(prime_event)


    # initialize the top root node
    while (len(top_event.p_children) + len(top_event.i_children)) < child_size:
        while len(top_event.p_children) < args.topprime:
            create_primary(top_event)

        create_inter(top_event)

    # initialize intermediate events
    while not inters_queue.empty():
        # get the intermediate event to intialize
        init_inter = inters_queue.get()

        # sample children size
        child_size = random.randint(min_children, max_children)

        while (len(init_inter.p_children) + len(init_inter.i_children)) < child_size:
            # case when the number of primary events is already satisfied
            if len(primary_vec) == args.nprimary:
                # reuse already initialized events only
                init_inter.p_children.add(random.choice(primary_vec))
                continue

            # sample inter events vs. primary events
            s_ratio = random.random()
            if s_ratio < (1.0/(1 + args.ratio)):
                create_inter(init_inter)
            else:
                # create a primary event
                # sample reuse
                s_reuse = random.random()
                if s_reuse < args.reuse and not len(primary_vec) == 0:
                    # reuse an already initialized primary event
                    init_inter.p_children.add(random.choice(primary_vec))
                else:
                    create_primary(init_inter)

            # corner case when not enough new primary events initialized, but
            # ther are no more intemediate events due to low ratio.
            if inters_queue.empty() and (len(primary_vec) < args.nprimary):
                # initialize one more intermediate event
                # this is a naive implementation, so
                # there might be another algorithm in future.
                create_inter(init_inter)

    # write output files
    tree_file = args.out + ".scramf"
    prob_file = args.out + ".scramp"

    t_file = open(tree_file, "w")
    p_file = open(prob_file, "w")

    t_file.write("# This is an input file for SCRAM. It is autogenerated with\n"\
        "# the following parameters:\n"\
        "# the seed of a random number generator: " + str(args.seed) + "\n"\
        "# the number of unique primary events: " + str(args.nprimary) + "\n"\
        "# the average number of children per intermediate event: " + str(args.nchildren) + "\n"\
        "# primary events to intermediate events ratio per new node: " + str(args.ratio) + "\n"\
        "# approximate percentage of repeated primary event in the tree: " + str(args.reuse) + "\n"\
        "# maximum probability for primary events: " + str(args.maxprob) + "\n"\
        "# minimum probability for primary events: " + str(args.minprob) + "\n"\
        "# minimal number of primary events for a root node: " + str(args.topprime) + "\n"\
        "# name of a file without extension to write the fault tree: " + str(args.out) + "\n"\
        )
    t_file.write("\n# ================= Start Fault Tree ===================\n")

    def write_node(inter_event, o_file):
        """Print children for the intermediate event.
        Note that it also updates the queue for intermediate events.
        """
        # print primary events first
        for p in inter_event.p_children:
            o_file.write("{\n    parent        " + inter_event.id + "\n"\
                        "    id            " + p.id +"\n"\
                        "    type          " + p.type + "\n}\n"
                        )
        # print intermediate events
        for i in inter_event.i_children:
            o_file.write("{\n    parent        " + inter_event.id + "\n"\
                        "    id            " + i.id +"\n"\
                        "    type          " + i.gate + "\n}\n"
                        )
            # update the queue
            inters_queue.put(i)

    # write top event and update queue of intermediate events
    # write top event and update queue of intermediate events
    t_file.write("{\n    parent        None\n"\
                "    id            " + top_event.id +"\n"\
                "    type          " + top_event.gate + "\n}\n"
                )
    write_node(top_event, t_file)

    # proceed with intermediate events
    while not inters_queue.empty():
        i_event = inters_queue.get()
        write_node(i_event, t_file)

    t_file.write("\n# ================= End Fault Tree ===================\n")

    t_file.close()


    # print probabilities of primary events
    p_file.write("# This is a probability input file accompanying a fault tree\n"\
                 "# written in " + tree_file + "\n"\
                 )
    p_file.write("\n# ================= Start Probabilities ===================\n")
    p_file.write("{\n")
    for p in primary_vec:
        p_file.write("    " + p.id + "    " + str(p.prob) + "\n")

    p_file.write("}\n")
    p_file.write("\n# ================= End Probabilities ===================\n")
    p_file.close()

if __name__ == "__main__":
    main()
