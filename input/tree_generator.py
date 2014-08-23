#! /usr/bin/env python

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
                        default=10)

    nchildren = "the average number of children per intermediate event"
    parser.add_argument("-c", "--nchildren", type=int, help=nchildren,
                        default=3)

    ratio = "primary events to intermediate events ratio per new node"
    parser.add_argument("-r", "--ratio", type=float, help=ratio,
                        default=2)

    reuse = "approximate percentage of repeated primary event in the tree"
    parser.add_argument("-u", "--reuse", type=float, help=reuse,
                        default=0.1)

    maxprob = "maximum probability for primary events"
    parser.add_argument("-x", "--maxprob", type=float, help=maxprob,
                        default=0.1)

    minprob = "minimum probability for primary events"
    parser.add_argument("-m", "--minprob", type=float, help=minprob,
                        default=0.001)

    topprime = "minimal number of primary events for a root node"
    parser.add_argument("--topprime", type=int, help=topprime,
                        default=0)

    ctop = "minimal number of children for a root node"
    parser.add_argument("-n", "--ctop", type=int, help=ctop,
                        default=0)

    out = "name of a file without extension to write the fault tree"
    parser.add_argument("-o", "--out", help=out, default="fta_tree")

    args = parser.parse_args()

    # Check for validity of arguments
    if args.maxprob < args.minprob:
        raise args.ArgumentTypeError("Max probability < Min probability")

    if args.topprime > args.nprimary:
        raise args.ArgumentTypeError("Topprime > number of total primary events")

    if args.topprime > args.ctop:
        raise args.ArgumentTypeError("Topprime > number of children for top")

    if args.ctop < 0 or\
            args.topprime < 0 or\
            args.ratio < 0 or\
            args.nchildren < 0 or\
            args.nprimary < 0 or\
            args.minprob < 0 or\
            args.maxprob < 0 or\
            args.reuse < 0:
        raise args.ArgumentTypeError("Check for negative value for arguments.")

    if args.reuse > 0.9 or\
            args.maxprob > 1 or\
            args.minprob > 1:
        raise args.ArgumentTypeError("Too big value for some of ratio args.")


    # Set the seed for this tree generator
    random.seed(args.seed)

    # Container for created primary events
    primary_vec = []

    # Supported types of gates
    types = ["or", "and"]

    # Minimum number of children per intermediate event
    min_children = 2

    # Maximum number of children
    max_children = args.nchildren * 2 - min_children

    # Tree generation
    # Start with a top event
    top_event = TopEvent()
    top_event.id = "TopEvent"
    top_event.gate = random.choice(types)
    child_size = random.randint(min_children, max_children)

    # Configuring child size for the top event
    if args.ctop != 0:
        child_size = args.ctop
    elif child_size < args.topprime:
        child_size = args.topprime

    # Container for not yet initialized intermediate events
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


    # Initialize the top root node
    while (len(top_event.p_children) + len(top_event.i_children)) < child_size:
        while len(top_event.p_children) < args.topprime:
            create_primary(top_event)

        create_inter(top_event)

    # Initialize intermediate events
    while not inters_queue.empty():
        # Get the intermediate event to intialize
        init_inter = inters_queue.get()

        # Sample children size
        child_size = random.randint(min_children, max_children)

        while (len(init_inter.p_children) + len(init_inter.i_children)) < child_size:
            # Case when the number of primary events is already satisfied
            if len(primary_vec) == args.nprimary:
                # Reuse already initialized events only
                init_inter.p_children.add(random.choice(primary_vec))
                continue

            # Sample inter events vs. primary events
            s_ratio = random.random()
            if s_ratio < (1.0/(1 + args.ratio)):
                create_inter(init_inter)
            else:
                # Create a primary event
                # Sample reuse
                s_reuse = random.random()
                if s_reuse < args.reuse and not len(primary_vec) == 0:
                    # Reuse an already initialized primary event
                    init_inter.p_children.add(random.choice(primary_vec))
                else:
                    create_primary(init_inter)

            # Corner case when not enough new primary events initialized, but
            # ther are no more intemediate events due to low ratio.
            if inters_queue.empty() and (len(primary_vec) < args.nprimary):
                # Initialize one more intermediate event.
                # This is a naive implementation, so
                # there might be another algorithm in future.
                create_inter(init_inter)

    # Write output files
    tree_file = args.out + ".xml"

    t_file = open(tree_file, "w")

    t_file.write("<?xml version=\"1.0\"?>\n");
    t_file.write("<!--\nThis is an input file for SCRAM. It is autogenerated with\n"\
        "The following parameters:\n"\
        "The seed of a random number generator: " + str(args.seed) + "\n"\
        "The number of unique primary events: " + str(args.nprimary) + "\n"\
        "The average number of children per intermediate event: " + str(args.nchildren) + "\n"\
        "Primary events to intermediate events ratio per new node: " + str(args.ratio) + "\n"\
        "Approximate percentage of repeated primary event in the tree: " + str(args.reuse) + "\n"\
        "Maximum probability for primary events: " + str(args.maxprob) + "\n"\
        "Minimum probability for primary events: " + str(args.minprob) + "\n"\
        "Minimal number of primary events for a root node: " + str(args.topprime) + "\n"\
        "Name of a file without extension to write the fault tree: " + str(args.out) + "\n"\
        "-->\n\n"\
        )
    t_file.write("<opsa-mef>\n");
    t_file.write("<define-fault-tree name=\"Autogenerated\">\n")

    def write_node(inter_event, o_file):
        """Print children for the intermediate event.
        Note that it also updates the queue for intermediate events.
        """

        o_file.write("<define-gate name=\"" + inter_event.id + "\">\n")
        o_file.write("<" + inter_event.gate + ">\n")
        # Print primary events
        for p in inter_event.p_children:
            o_file.write("<basic-event name=\"" + p.id + "\"/>\n")

        # Print intermediate events
        for i in inter_event.i_children:
            o_file.write("<gate name=\"" + i.id + "\"/>\n")
            # Update the queue
            inters_queue.put(i)

        o_file.write("</" + inter_event.gate + ">\n")
        o_file.write("</define-gate>\n")

    # Write top event and update queue of intermediate events
    # Write top event and update queue of intermediate events
    write_node(top_event, t_file)

    # Proceed with intermediate events
    while not inters_queue.empty():
        i_event = inters_queue.get()
        write_node(i_event, t_file)

    t_file.write("</define-fault-tree>\n")

    # Print probabilities of primary events
    t_file.write("<model-data>\n")
    for p in primary_vec:
        t_file.write("<define-basic-event name=\"" + p.id + "\">\n"\
                     "<float value=\"" + str(p.prob) + "\"/>\n"\
                     "</define-basic-event>\n")


    t_file.write("</model-data>\n")
    t_file.write("</opsa-mef>")
    t_file.close()

if __name__ == "__main__":
    main()
